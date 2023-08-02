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
    std::thread reqs_thread(handle_leader_reqs);
    
    // wait until the reqs thread sets the endpoint
    std::thread ping_thread(handle_pings);

    reqs_thread.join();
    ping_thread.join();
}


void handle_leader_reqs() {
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

        std::string msg = request.to_string();
    
        
        if (cmd::nodeCmds(cmd::extract_name(msg)) == cmd::NodeCMDType::Kill && 
            cmd::extract_key(msg) == worker_pid
        ) {
            socket.send(zmq::buffer("OK"), zmq::send_flags::none);
            exit(EXIT_SUCCESS);
        }

        Command cmd { msg };
        std::string parsed = cmd.parse_cmd();

        socket.send(zmq::buffer(parsed), zmq::send_flags::none);
      
        if (stop) {
            std::cerr << "Stopping worker node pid " << worker_pid << std::endl;
            exit(EXIT_SUCCESS);
        }
    }
}

void handle_pings() {
    // wait until the reqs thread sets the endpoint
    endpoint_mutex.lock();

    std::string worker_pid = std::to_string(getpid());

    // open connection to leader to send endpoint of this
    zmq::context_t leader_context{1};
    zmq::socket_t leader_socket{leader_context, zmq::socket_type::req};

    leader_socket.connect("tcp://localhost:" + std::to_string(internal_port));
    // send pid and endpoint to leader
    leader_socket.set(zmq::sockopt::rcvtimeo, 1000);
    leader_socket.send(zmq::buffer(worker_pid + " " + endpoint), zmq::send_flags::none);

    // verify the endpoint was successfully recieved
    try {
        zmq::message_t reply;
        zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
        if (!res.has_value()) {
            throw std::strerror(errno);
        }
    } catch (...) {
        std::cerr << "Error communicating with leader: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);;
    }

    while (true) {
        // send ping to leader
        leader_socket.send(zmq::message_t(), zmq::send_flags::none);

        // verify the ping was successfully recieved
        try {
            zmq::message_t reply;
            zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
            if (!res.has_value()) {
                throw std::strerror(errno);
            }

            ring.add_all(reply.to_string());    
            std::this_thread::sleep_for(1000ms);
        } catch (...) {
            std::cerr << "Leader did not reply to ping" << std::endl;
            start_leader_election();
        }
    }
}

void start_leader_election() {
    std::cout << "Calling an election..." << std::endl;
}
