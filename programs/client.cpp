#include <string>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <zmq.hpp>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Enter a port number: e.g. client/client 5555." << std::endl;
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);

    if (port <= 0) {
        std::cout << "Enter a valid port number: e.g. client/client 5555." << std::endl;
        return EXIT_FAILURE;
    }
    
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::req};
    socket.connect("tcp://localhost:" + std::to_string(port));
    socket.set(zmq::sockopt::rcvtimeo, 1000);
    std::cout << "Client started!" << std::endl;

    while (true) {
        std::cout << "> ";
        std::flush(std::cout);

        std::string input;
        std::getline(std::cin, input);
    
        socket.send(zmq::buffer(input), zmq::send_flags::none);
        
        try {
            zmq::message_t reply{};
            zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
            if (!res.has_value()) {
                throw std::strerror(errno);
            }

            std::cout << reply.to_string() << std::endl;
        } catch (...) {
            std::cout << "Error communicating with server: " << std::strerror(errno) << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
