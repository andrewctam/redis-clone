#include <iostream>
#include <zmq.hpp>
#include <unistd.h>

#include "command.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "unix_times.hpp"
#include "consistent-hashing.hpp"
#include "leader.hpp"

using namespace std::chrono_literals;

void start_leader() {
    // internal socket for talking to worker nodes
    zmq::context_t internal_context{1};
    zmq::socket_t internal_socket{internal_context, zmq::socket_type::rep};
    internal_socket.bind("tcp://*:" + std::to_string(internal_port));
    internal_socket.set(zmq::sockopt::rcvtimeo, 200);

    // dealer socket for talking to all worker nodes at once
    zmq::context_t dealer_context{1};
    zmq::socket_t dealer_socket(dealer_context, zmq::socket_type::dealer);
    dealer_socket.set(zmq::sockopt::rcvtimeo, 200);

    // client socket for client requests
    zmq::context_t client_context{1};
    zmq::socket_t client_socket{client_context, zmq::socket_type::rep};
    client_socket.bind("tcp://*:" + std::to_string(client_port));

    // ring for consistent hashing and storing worker nodes
    std::string leader_pid = std::to_string(getpid());
    ConsistentHashing ring {};
    ring.add(leader_pid, internal_socket.get(zmq::sockopt::last_endpoint), true);

    std::cerr << "Started server with leader node with pid " << leader_pid << " on " 
        << client_socket.get(zmq::sockopt::last_endpoint)
        << std::endl;


    constexpr int INTERNAL = 0;
    constexpr int CLIENT = 1;
    zmq::pollitem_t sockets[] = {
        {internal_socket, 0, ZMQ_POLLIN, 0},
        {client_socket, 0, ZMQ_POLLIN, 0},
    };

    while (true) {
        zmq::poll(sockets, 2, -1);
        
        if (sockets[INTERNAL].revents & ZMQ_POLLIN) {
            handle_new_node(internal_socket, dealer_socket, ring);
        }

        if (sockets[CLIENT].revents & ZMQ_POLLIN) {
            handle_client_request(client_socket, dealer_socket, ring);
        }

        if (stop) {
            std::cerr << "Stopping leader node pid " << leader_pid << std::endl;
            return;
        }
    }
}


void handle_new_node(zmq::socket_t &internal_socket, zmq::socket_t &dealer_socket, ConsistentHashing &ring) {
    try {
        zmq::message_t reply{};
        zmq::recv_result_t res = internal_socket.recv(reply, zmq::recv_flags::none);

        std::string payload = reply.to_string();

        // add to ring
        int sep = payload.find(" ");
        std::string pid = payload.substr(0, sep);
        std::string endpoint = payload.substr(sep + 1); 
        ring.add(pid, endpoint, false);

        // add to dealer
        dealer_socket.connect(endpoint);

        std::cerr << "Connected to worker node with pid: " 
        << pid << " and endpoint: " << endpoint
        << std::endl;

        internal_socket.send(zmq::buffer("OK"), zmq::send_flags::none);
    } catch (...) {
        if (ring.size() == 1) { //only master node
            std::cerr << "Failed to find any child nodes, will only use master node." << std::endl;
        }
    }
}

void handle_client_request(zmq::socket_t &client_socket, zmq::socket_t &dealer_socket, ConsistentHashing &ring) {
    std::string leader_pid = std::to_string(getpid());

    zmq::message_t request;
    zmq::recv_result_t res = client_socket.recv(request, zmq::recv_flags::none);

    std::string reply = "";
    std::string msg = request.to_string();

    if (monitoring) {
        std::cerr << msg << std::endl; 
    }
    
    // does this command require asking all the nodes?
    std::string name = cmd::extract_name(msg);
    bool isNodeCmd = cmd::nodeCmds(name);
    bool shouldAddAll = cmd::addAll(name);
    bool shouldConcatAll = cmd::concatAll(name);
    bool shouldAskAll = cmd::askAll(name);

    if (isNodeCmd) {
        if (name == "nodes") {
            reply = ring.to_user_string();
        } else if (name == "kill") {
            ServerNode *node = ring.get_by_pid(cmd::extract_key(msg));
            if (!node) {
                reply = "Node not found";
            } else if (node->pid == leader_pid) {
                client_socket.send(zmq::buffer("OK"), zmq::send_flags::none);
                exit(EXIT_SUCCESS);
            } else {
                node->socket->send(zmq::buffer("kill"), zmq::send_flags::none);
                zmq::recv_result_t res = node->socket->recv(request, zmq::recv_flags::none);
                reply = request.to_string();
            }
        } else if (name == "create") {
            int pid = fork();
            if (pid == 0) {
                execlp("./worker_node", "./worker_node", nullptr);
                exit(EXIT_FAILURE);
            } else {
                reply = "Worker node created with pid " + std::to_string(pid);
            }
        }
    } else if (shouldAddAll || shouldConcatAll || shouldAskAll) {
        for (int i = 0; i < ring.size() - 1; i++) {
            dealer_socket.send(zmq::message_t(), zmq::send_flags::sndmore);
            dealer_socket.send(zmq::buffer(msg), zmq::send_flags::none);
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
                zmq::recv_result_t res = dealer_socket.recv(dealer_request, zmq::recv_flags::none);
                // actual result
                res = dealer_socket.recv(dealer_request, zmq::recv_flags::none);

                count++;
                if (shouldAddAll) {
                    try {
                        sum += stoi(dealer_request.to_string());
                    } catch (...) {
                        std::cerr << "Error with this cmd: " << msg << std::endl;
                    }
                } else if (shouldConcatAll) {
                    ss << dealer_request.to_string() << " ";
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
}