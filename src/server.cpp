#include <iostream>
#include <zmq.hpp>
#include <errno.h>

#include "command.h"
#include "lru_cache.h"
#include "server.h"
#include "unix_times.h"
#include "consistent-hashing.h"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;

using namespace std::chrono_literals;


void start_leader() {
    // internal context for talking to worker nodes
    zmq::context_t internal_context{1};
    zmq::socket_t internal_socket{internal_context, zmq::socket_type::rep};
    internal_socket.bind("tcp://*:" + std::to_string(CLIENT_PORT + 10000));
    internal_socket.set(zmq::sockopt::rcvtimeo, 200);

    // dealer context for talking to all worker nodes at once
    zmq::context_t dealer_context(1);
    zmq::socket_t dealer_socket(dealer_context, zmq::socket_type::dealer);
    dealer_socket.set(zmq::sockopt::rcvtimeo, 200);

    // ring for consistent hashing and storing worker nodes
    std::string leader_pid = std::to_string(getpid());
    ConsistentHashing ring {};
    ring.add(leader_pid, internal_socket.get(zmq::sockopt::last_endpoint));

    // recieve worker node discovery connections, stop after 200ms of no discovery
    while (true) {
         try {
            zmq::message_t reply{};
            zmq::recv_result_t res = internal_socket.recv(reply, zmq::recv_flags::none);

            
            std::string payload = reply.to_string();

            // add to ring
            int sep = payload.find(" ");
            std::string pid = payload.substr(0, sep);
            std::string endpoint = payload.substr(sep + 1); 
            ring.add(pid, endpoint);

            // add to dealer
            dealer_socket.connect(endpoint);

            std::cout << "Connected to worker node with pid: " 
            << pid << " and endpoint: " << endpoint
            << std::endl;

            internal_socket.send(zmq::buffer("OK"), zmq::send_flags::none);
        } catch (...) {
            if (ring.size() == 1) { //only master node
                std::cout << "Failed to find any child nodes, will only use master node." << std::endl;
            }

            break;
        }
    }

    
        
    // create socket for client requests
    zmq::context_t client_context{1};
    zmq::socket_t client_socket{client_context, zmq::socket_type::rep};
    client_socket.bind("tcp://*:" + std::to_string(CLIENT_PORT));

    std::cout << "Started server with leader node with pid " << leader_pid << " on " 
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
        bool shouldAddAll = cmd::addAll(name);
        bool shouldConcatAll = cmd::concatAll(name);
        bool shouldAskAll = cmd::askAll(name);

        if (shouldAddAll || shouldConcatAll || shouldAskAll) {
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
                    std::cout << "Error with this cmd: " << msg << std::endl;
                }
            } else if (shouldConcatAll) {
                std::cout << parsed << std::endl;
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
                            std::cout << "Error with this cmd: " << msg << std::endl;
                        }
                    } else if (shouldConcatAll) {
                        ss << dealer_request.to_string() << " ";
                    }
                } catch (...) {
                    std::cout << "Recv " << count << " out of " << ring.size() << std::endl;
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
            std::cout << "Stopping leader node pid " << leader_pid << std::endl;
            return;
        }
    }
}


void start_worker() {
    // open connection to leader to send endpoint of this
    zmq::context_t leader_context{1};
    zmq::socket_t leader_socket{leader_context, zmq::socket_type::req};
    leader_socket.connect("tcp://localhost:" + std::to_string(LEADER_PORT));

    // open a endpoint
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind("tcp://*:0");

    std::string endpoint = socket.get(zmq::sockopt::last_endpoint);
    std::string pid = std::to_string(getpid());

    // send pid and endpoint to leader
    leader_socket.set(zmq::sockopt::rcvtimeo, 1000);
    leader_socket.send(zmq::buffer(pid + " " + endpoint), zmq::send_flags::none);

    // verify the endpoint was successfully recieved
    try {
        zmq::message_t reply{};
        zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
        if (!res.has_value()) {
            throw std::strerror(errno);
        }
    } catch (...) {
        std::cout << "Error communicating with leader: " << std::strerror(errno) << std::endl;
        return;
    }

    
    // listen for requests from the leader
    while (true) {
        zmq::message_t request;
        zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);

        Command cmd { request.to_string() };
        std::string res2 = cmd.parse_cmd();

        socket.send(zmq::buffer(res2), zmq::send_flags::none);
      
        if (stop) {
            std::cout << "Stopping worker node pid " << getpid() << std::endl;
            return;
        }
    }
}
