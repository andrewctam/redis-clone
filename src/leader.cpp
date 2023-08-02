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

using namespace std::chrono_literals;

void start_leader() {    
    std::thread client_thread(handle_client_requests);
    std::thread internal_thread(handle_internal_requests);

    client_thread.join();
    internal_thread.join();
}

void handle_internal_requests() {
    std::string leader_pid = std::to_string(getpid());

    // internal socket for talking to worker nodes
    zmq::context_t internal_context{1};
    zmq::socket_t internal_socket{internal_context, zmq::socket_type::rep};
    internal_socket.bind("tcp://*:" + std::to_string(internal_port));
    internal_socket.set(zmq::sockopt::rcvtimeo, 200);
    ring.add(leader_pid, internal_socket.get(zmq::sockopt::last_endpoint), true);

    
    while (true) {
        try {
            zmq::message_t reply{};
            zmq::recv_result_t res = internal_socket.recv(reply, zmq::recv_flags::none);

            // new connections contain a message, pings are empty
            if (reply.to_string().size() > 0) {
                 std::string payload = reply.to_string();

                // add to ring
                int sep = payload.find(" ");
                std::string pid = payload.substr(0, sep);
                std::string endpoint = payload.substr(sep + 1); 
                ring.add(pid, endpoint, false);

                std::cerr << "Connected to worker node with pid: " << pid 
                    << " and endpoint: " << endpoint << std::endl;
            }
           
            //send latest connections
            internal_socket.send(zmq::buffer(ring.to_internal_string()), zmq::send_flags::none);
        } catch (...) {
            if (ring.size() == 1) { //only master node
                std::cerr << "Failed to find any child nodes, will only use master node." << std::endl;
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

    std::cerr << "Started server with leader node with pid " << leader_pid << " on " 
        << client_socket.get(zmq::sockopt::last_endpoint)
        << std::endl;

    while (true) {
        zmq::message_t request;
        zmq::recv_result_t res = client_socket.recv(request, zmq::recv_flags::none);

        std::string reply = "";
        std::string msg = request.to_string();

        if (monitoring) {
            std::cerr << msg << std::endl; 
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
                        execlp("./worker_node", "./worker_node", nullptr);
                        exit(EXIT_FAILURE);
                    } else {
                        reply = "Worker node created with pid " + std::to_string(pid);
                    }
                    break;
                }
                case cmd::NodeCMDType::Kill: {
                    ServerNode *node = ring.get_by_pid(cmd::extract_key(msg));
                    if (!node) {
                        reply = "Node not found";
                    } else if (node->pid == leader_pid) {
                        client_socket.send(zmq::buffer("OK"), zmq::send_flags::none);
                        exit(EXIT_SUCCESS);
                    } else {
                        node->socket->send(zmq::buffer(msg), zmq::send_flags::none);
                        zmq::recv_result_t res = node->socket->recv(request, zmq::recv_flags::none);
                        reply = request.to_string();
                    }
                    break;
                }
                default:
                    break;
            }
        } else if (shouldAddAll || shouldConcatAll || shouldAskAll) {
            for (int i = 0; i < ring.size() - 1; i++) {
                ring.dealer_socket->send(zmq::message_t(), zmq::send_flags::sndmore);
                ring.dealer_socket->send(zmq::buffer(msg), zmq::send_flags::none);
            }
            int sum = 0;
            std::stringstream ss;

            // process onmaster node
            Command cmd { request.to_string() };
            std::string parsed = cmd.parse_cmd();

            if (shouldAddAll) {
                try {
                    sum += stoi(parsed);
                } catch (...) {
                    std::cerr << "Error with this cmd: " << msg << std::endl;
                }
            } else if (shouldConcatAll) {
                if (parsed.size() > 0) {
                    ss << parsed << " ";
                }
            }

            // ask for worker node responses
            // stop after all responses (or timeout)
            int count = 1;
            while (count < ring.size()) {
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
                            std::cerr << "Error with this cmd: " << msg << std::endl;
                        }
                    } else if (shouldConcatAll) {
                        std::string str = dealer_request.to_string();
                        if (str.size() > 0) {
                            ss << str << " ";
                        }
                    }
                } catch (...) {
                    std::cerr << "Recv " << count << " out of " << ring.size() << std::endl;
                }
            }

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
            }

            if (worker && worker->pid != leader_pid) {
                //request goes to another worker
                worker->socket->send(zmq::buffer(msg), zmq::send_flags::none);
                zmq::recv_result_t res = worker->socket->recv(request, zmq::recv_flags::none);
                reply = request.to_string();
            } else { 
                //request can be fuffiled by leader
                Command cmd { request.to_string() };
                reply = cmd.parse_cmd();
            }
        }
        

        client_socket.send(zmq::buffer(reply), zmq::send_flags::none);
        if (stop) {
            std::cerr << "Stopping leader node pid " << leader_pid << std::endl;
            exit(EXIT_SUCCESS);
        }
    }
}