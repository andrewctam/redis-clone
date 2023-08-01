#include <iostream>
#include <zmq.hpp>
#include <errno.h>

#include "command.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "unix_times.hpp"
#include "consistent-hashing.hpp"
#include "worker.hpp"

using namespace std::chrono_literals;

void start_worker() {
    // open connection to leader to send endpoint of this
    zmq::context_t leader_context{1};
    zmq::socket_t leader_socket{leader_context, zmq::socket_type::req};
    leader_socket.connect("tcp://localhost:" + std::to_string(internal_port));

    // open a endpoint
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::rep};
    socket.bind("tcp://*:0");

    std::string endpoint = socket.get(zmq::sockopt::last_endpoint);
    std::string worker_pid = std::to_string(getpid());

    // send pid and endpoint to leader
    leader_socket.set(zmq::sockopt::rcvtimeo, 1000);
    leader_socket.send(zmq::buffer(worker_pid + " " + endpoint), zmq::send_flags::none);

    // verify the endpoint was successfully recieved
    try {
        zmq::message_t reply{};
        zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
        if (!res.has_value()) {
            throw std::strerror(errno);
        }
    } catch (...) {
        std::cerr << "Error communicating with leader: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);;
    }

    
    // listen for requests from the leader
    while (true) {
        zmq::message_t request;
        zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);

        std::string msg = request.to_string();

        if (msg == "kill") {
            socket.send(zmq::buffer("OK"), zmq::send_flags::none);
            exit(EXIT_SUCCESS);
        }

        Command cmd { msg };
        std::string parsed = cmd.parse_cmd();

        socket.send(zmq::buffer(parsed), zmq::send_flags::none);
      
        if (stop) {
            std::cerr << "Stopping worker node pid " << worker_pid << std::endl;
            exit(EXIT_SUCCESS);;
        }
    }
}

