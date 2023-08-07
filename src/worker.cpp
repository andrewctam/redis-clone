#include <thread>
#include <iostream>
#include <zmq.hpp>
#include <errno.h>
#include <string>

#include "command.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "unix_times.hpp"
#include "consistent-hashing.hpp"
#include "worker.hpp"

std::string endpoint;
std::mutex endpoint_mutex;

void start_worker() {
    // lock this mutex, will be unlocked once the reqs thread sets endpoint
    endpoint_mutex.lock();
    std::thread reqs_thread(handle_reqs);
    
    // wait until the reqs thread sets the endpoint
    std::thread ping_thread(handle_pings);

    reqs_thread.join();
    ping_thread.join();
}


void handle_reqs() {
    std::string worker_pid = std::to_string(getpid());

    // open an endpoint
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:0");

    // set endpoint and unlock mutex
    endpoint = socket.get(zmq::sockopt::last_endpoint);
    endpoint_mutex.unlock();

    // listen for requests from the leader
    while (true) {
        zmq::message_t request;
        zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);

        std::string response = "";
        std::string msg = request.to_string();

        if (msg.size() > 0) {
            std::string msg_body = msg.substr(1);
            switch(msg.at(0)) {
                case COMMAND: {
                    Command cmd { msg_body };
                    response = cmd.parse_cmd();
                    break;
                } 
                case NODE_COMMAND:
                    if (cmd::nodeCmds(cmd::extract_name(msg_body)) == cmd::NodeCMDType::Kill && 
                        cmd::extract_key(msg_body) == worker_pid
                    ) {
                        socket.send(zmq::buffer("OK"), zmq::send_flags::none);
                        std::cout << "Stopping " << worker_pid << std::endl;
                        exit(EXIT_SUCCESS);
                    }
                    break;
                case RING_UPDATE:
                    ring.update(msg_body);    
                    break;
                case CACHE_UPDATE: {
                    int old_size = cache.size();
                    cache.import(msg_body);
                    response = std::to_string(cache.size() - old_size);
                    break;
                }
                case ELECTION:
                    break;

                default:
                    response = "Missing type char";
            }
        }

        socket.send(zmq::buffer(response), zmq::send_flags::none);
      
        if (stop) {
            std::cout << "Stopping worker node pid " << worker_pid << std::endl;
            exit(EXIT_SUCCESS);
        }
    }
}

void handle_pings() {
    // wait until the reqs thread sets the endpoint
    endpoint_mutex.lock();
    std::string worker_pid = std::to_string(getpid());
    std::string ping = worker_pid + " " + endpoint;

    // open connection to leader to send endpoint of this
    zmq::context_t leader_context{1};
    zmq::socket_t leader_socket{leader_context, zmq::socket_type::req};

    leader_socket.connect("tcp://localhost:" + std::to_string(internal_port));
    // send pid and endpoint to leader
    leader_socket.set(zmq::sockopt::rcvtimeo, 1000);
    leader_socket.send(zmq::buffer(ping), zmq::send_flags::none);

    // verify the endpoint was successfully recieved
    try {
        zmq::message_t reply;
        zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
        if (!res.has_value()) {
            throw std::strerror(errno);
        }
    } catch (...) {
        std::cout << "Error communicating with leader: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);;
    }

    while (true) {
        leader_socket.send(zmq::message_t(ping), zmq::send_flags::none);
        // verify the ping was successfully recieved
        try {
            zmq::message_t reply;
            zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
            if (!res.has_value()) {
                throw std::strerror(errno);
            }

            ring.update(reply.to_string());    
            std::this_thread::sleep_for(1000ms);
        } catch (...) {
            std::cout << "Leader did not reply to ping" << std::endl;
            start_leader_election();
        }
    }
}

void start_leader_election() {
    std::cout << "Calling an election..." << std::endl;
}

void promote_to_leader() {
    std::cout << "Promoting to leader..." << std::endl;
    ring.set_up_dealer();
}
