#include <thread>
#include <iostream>
#include <zmq.hpp>
#include <unistd.h>
#include <string>

#include "command.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "unix_times.hpp"
#include "consistent-hashing.hpp"
#include "leader.hpp"
#include "worker.hpp"

using namespace std::chrono_literals;

void start_leader() {
    ring.set_up_dealer();
    
    std::thread client_thread(handle_client_requests);
    std::thread internal_thread(handle_internal_requests);
    std::thread cleanup_thread(handle_nodes_cleanup);

    client_thread.join();
    internal_thread.join();
    cleanup_thread.join();
}

void handle_nodes_cleanup() {
    while (true) {
        std::this_thread::sleep_for(1000ms);
        ring.clean_up_old_nodes();
    }
}

void handle_internal_requests() {
    std::string leader_pid = std::to_string(getpid());

    // internal socket for talking to worker nodes
    zmq::context_t internal_context{1};
    zmq::socket_t internal_socket{internal_context, zmq::socket_type::rep};
    internal_socket.bind("tcp://*:" + std::to_string(internal_port));
    ring.add(leader_pid, internal_socket.get(zmq::sockopt::last_endpoint), true);

    ServerNode *this_node = ring.get_by_pid(leader_pid);
    while (true) {
        try {
            zmq::message_t reply;
            zmq::recv_result_t res = internal_socket.recv(reply, zmq::recv_flags::none);
            bool new_added = false;
            std::string ping_msg = reply.to_string();

            ServerNode *next_node = nullptr;
            ServerNode *added = nullptr;
            bool wrap_around = false;

            int sep = ping_msg.find(" ");
            if (sep != std::string::npos) {
                std::string pid = ping_msg.substr(0, sep);
                std::string endpoint = ping_msg.substr(sep + 1); 

                ServerNode *cur = ring.get(pid + endpoint);
                if (cur && cur->pid == pid && cur->endpoint == endpoint) {
                    cur->refresh_last_ping();
                } else {
                    // add this to ring
                    new_added = true;

                    next_node = ring.get_next_node(this_node);
                    bool wrap_around = ring.is_begin(next_node);

                
                    added = ring.add(pid, endpoint, false);
                    
                    std::cout << "Connected to worker node with pid: " << pid 
                        << " and endpoint: " << endpoint << std::endl;
                }
            }

            ring.clean_up_old_nodes();
            std::string internal = ring.to_internal_string();

            //send latest connections, RING_UPDATE prefix not needed for ping
            internal_socket.send(zmq::buffer(internal), zmq::send_flags::none);

            if (new_added) {
                if (next_node && added && ring.get_next_node(this_node) == added) {
                    ring.send_extracted_cache(this_node, next_node, wrap_around);
                }

                //ring update needed for request
                ring.dealer_send(RING_UPDATE + internal);
                
                ring.mutex.lock();
                int count = 0;
                while (count < ring.dealer_connected) {
                    try {
                        zmq::message_t dealer_request;
                        // empty envelope
                        zmq::recv_result_t res = ring.dealer_socket->recv(dealer_request, zmq::recv_flags::none);
                        // actual result
                        res = ring.dealer_socket->recv(dealer_request, zmq::recv_flags::none);
                        count++;
                    } catch (...) {
                        std::cout << "Recv " << count << " out of " << ring.size() << std::endl;
                    }
                }
                ring.mutex.unlock();

            }
        } catch (...) {
            if (ring.size() == 1) { //only master node
                std::cout << "Failed to find any child nodes, will only use master node." << std::endl;
            }
        }
    }
}

void handle_client_requests() {
    std::string leader_pid = std::to_string(getpid());

    // client socket for client requests
    zmq::context_t client_context{1};
    zmq::socket_t client_socket{client_context, zmq::socket_type::rep};
    client_socket.bind("tcp://*:" + std::to_string(client_port));

    std::cout << "Started leader node with pid " << leader_pid << " on " 
        << client_socket.get(zmq::sockopt::last_endpoint)
        << std::endl;

    while (true) {
        zmq::message_t request;
        zmq::recv_result_t res = client_socket.recv(request, zmq::recv_flags::none);

        std::string reply = "";
        std::string msg = request.to_string();

        if (monitoring) {
            std::cout << msg << std::endl; 
        }
        
        // does this command require asking all the nodes?
        std::string name = cmd::extract_name(msg);
        cmd::NodeCMDType nodeCmd = cmd::nodeCmds(name);
        bool shouldAddAll = cmd::addAll(name);
        bool shouldConcatAll = cmd::concatAll(name);
        bool shouldAskAll = cmd::askAll(name);

        if (nodeCmd != cmd::NodeCMDType::Not) {
            switch(nodeCmd) {
                case cmd::NodeCMDType::Nodes: {
                    reply = ring.to_user_string();
                    break;
                }
                case cmd::NodeCMDType::Create: {
                    int pid = fork();
                    if (pid == 0) {
                        char i_port[100], c_port[100];
                        snprintf(i_port, sizeof(i_port), "%d", internal_port);
                        snprintf(c_port, sizeof(c_port), "%d", client_port);
                        execlp("./node", "./node", "-w", "-i", i_port, "-c", c_port, nullptr);
                        exit(EXIT_FAILURE);
                    } else {
                        reply = "Worker node created with pid " + std::to_string(pid);
                    }
                    break;
                }
                case cmd::NodeCMDType::Kill: {
                    std::string key = cmd::extract_key(msg);
                    if (key == "leader" || key == leader_pid) {
                        client_socket.send(zmq::buffer("OK"), zmq::send_flags::none);
                        exit(EXIT_SUCCESS);
                    }

                    ServerNode *node = ring.get_by_pid(key);
                    if (!node) {
                        reply = "Node not found";
                    } else {
                        node->send(NODE_COMMAND + msg);
                        node->recv(request);
                        reply = request.to_string();
                    }
                    break;
                }
                default:
                    break;
            }
        } else if (shouldAddAll || shouldConcatAll || shouldAskAll) {
            ring.dealer_send(COMMAND + msg);

            int sum = 0;
            std::stringstream ss;

            // process onmaster node
            Command cmd { request.to_string() };
            std::string parsed = cmd.parse_cmd();

            if (shouldAddAll) {
                try {
                    sum += stoi(parsed);
                } catch (...) {
                    std::cout << "Error with this cmd: " << msg << std::endl;
                }
            } else if (shouldConcatAll) {
                if (parsed.size() > 0) {
                    ss << parsed << " ";
                }
            }

            // ask for worker node responses
            // stop after all responses (or timeout)
            ring.mutex.lock();
            int count = 0;
            while (count < ring.dealer_connected) {
                try {
                    zmq::message_t dealer_request;
                    // empty envelope
                    zmq::recv_result_t res = ring.dealer_socket->recv(dealer_request, zmq::recv_flags::none);
                    // actual result
                    res = ring.dealer_socket->recv(dealer_request, zmq::recv_flags::none);

                    count++;
                    if (shouldAddAll) {
                        try {
                            sum += stoi(dealer_request.to_string());
                        } catch (...) {
                            std::cout << "Error with this cmd: " << msg << std::endl;
                        }
                    } else if (shouldConcatAll) {
                        std::string str = dealer_request.to_string();
                        if (str.size() > 0) {
                            ss << str << " ";
                        }
                    }
                } catch (...) {
                    std::cout << "Recv " << count << " out of " << ring.size() << std::endl;
                }
            }
            ring.mutex.unlock();
            if (shouldAddAll) {
                reply = std::to_string(sum);
            } else if (shouldConcatAll) {
                reply = ss.str();
            }
        } else {
            ServerNode *worker = nullptr;
            std::string key = cmd::extract_key(msg);
            if (key != "") {
                worker = ring.get(key);
                if (monitoring) {
                    std::cout << key << " [" << hash_function(key)
                        << "] in to Node [" << worker->hash << "] with pid " 
                        << worker->pid  << std::endl;
                }
            }

            if (worker && worker->pid != leader_pid) {
                //request goes to another worker
                worker->send(COMMAND + msg);
                if (worker->recv(request)) {
                    reply = request.to_string();
                } else {
                    reply = "FAILED";
                }
            } else { 
                //request can be fuffiled by leader
                Command cmd { request.to_string() };
                reply = cmd.parse_cmd();
            }
        }
        

        client_socket.send(zmq::buffer(reply), zmq::send_flags::none);
        if (stop) {
            std::cout << "Stopping leader node pid " << leader_pid << std::endl;
            exit(EXIT_SUCCESS);
        }
    }
}