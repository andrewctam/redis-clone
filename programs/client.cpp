#include <string>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <zmq.hpp>
#include <getopt.h>

int main(int argc, char *argv[]) {
    int server_port = 5555;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"port", required_argument, 0, 'p'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-hp:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'h':
                std::cout 
                    << "Redis clone client. Usage: ./client [OPTIONS]\n"
                    << "-h, --help: displays this menu!\n"
                    << "-p, --port: port used by server for client connections. Default is 5555"
                    << std::endl;

                return EXIT_SUCCESS;
            case 'p':
                if (!optarg) {
                    std::cout << "Must enter a port" << std::endl;
                    return EXIT_FAILURE;
                }
                server_port = atoi(optarg);
                if (server_port <= 0) {
                    std::cout << "Port must be greater than 0!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            default:
                return EXIT_FAILURE;
        }
    }
    
    zmq::context_t context{1};
    zmq::socket_t socket{context, zmq::socket_type::req};
    socket.connect("tcp://localhost:" + std::to_string(server_port));
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
