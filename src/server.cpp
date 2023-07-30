#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <zmq.hpp>

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
    ConsistentHashing ring {};

    zmq::context_t internal_context{1};
    zmq::socket_t internal_socket{internal_context, zmq::socket_type::rep};
    internal_socket.bind("tcp://*:" + std::to_string(CLIENT_PORT + 10000));
    internal_socket.set(zmq::sockopt::rcvtimeo, 1000);
    while (true) {
         try {
            zmq::message_t reply{};
            zmq::recv_result_t res = internal_socket.recv(reply, zmq::recv_flags::none);

            std::string payload = reply.to_string();
            int sep = payload.find(" ");
            std::string pid = payload.substr(0, sep);
            std::string endpoint = payload.substr(sep + 1); 
            ring.add(pid, endpoint);

            std::cout << "Connected to worker node with pid: " 
            << pid << " and endpoint: " << endpoint
            << std::endl;

            internal_socket.send(zmq::buffer("OK"), zmq::send_flags::none);
        } catch (...) {
            if (ring.size() == 0) {
                std::cout << "Failed to find any child nodes" << std::endl;
                return;
            }

            break;
        }
    }
        

    zmq::context_t client_context{1};
    zmq::socket_t client_socket{client_context, zmq::socket_type::rep};
    client_socket.bind("tcp://*:" + std::to_string(CLIENT_PORT));

    std::cout << "Started server with leader node pid:" << getpid() << " on: " 
        << client_socket.get(zmq::sockopt::last_endpoint)
        << std::endl;

    
    while (true) {
        zmq::message_t request;
        zmq::recv_result_t res = client_socket.recv(request, zmq::recv_flags::none);

        std::string msg = request.to_string();
        std::string key = cmd::extract_key(msg);

        ServerNode *worker = nullptr;
        if (key != "") {
            worker = ring.get(key);
            std::cout << worker->pid << std::endl; 
        }

        if (worker) {
            worker->socket->send(zmq::buffer(msg), zmq::send_flags::none);

            zmq::recv_result_t res = worker->socket->recv(request, zmq::recv_flags::none);
        }

        client_socket.send(zmq::buffer(request.to_string()), zmq::send_flags::none);
      
        if (stop) {
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

        std::string msg = request.to_string();
        Command cmd { msg };


        socket.send(zmq::buffer(cmd.parse_cmd()), zmq::send_flags::none);
      
        if (stop) {
            return;
        }
    }
}
