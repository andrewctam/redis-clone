#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <zmq.hpp>

#include "command.h"
#include "lru_cache.h"
#include "server.h"
#include "unix_times.h"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;

using namespace std::chrono_literals;
void start_server()  {
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::rep};

    socket.bind("tcp://*:5555");

    std::cout << "Server started!" << std::endl;

    while (true) {
        zmq::message_t request;

        socket.recv(request, zmq::recv_flags::none);
        std::string msg = request.to_string();

        Command cmd { msg };

        socket.send(zmq::buffer(cmd.parse_cmd()), zmq::send_flags::none);
    }
}

