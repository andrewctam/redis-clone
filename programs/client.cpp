#include <string>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <zmq.hpp>
#include <getopt.h>
#include <iostream>
#include <string>
#include <random>

std::string randomStr() {
    int minLength = 2;
    int maxLength = 10;

    const std::string charSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lengthDistribution(minLength, maxLength);
    std::uniform_int_distribution<> charDistribution(0, charSet.size() - 1);

    int length = lengthDistribution(gen);

    std::string result;
    for (int i = 0; i < length; ++i) {
        result += charSet[charDistribution(gen)];
    }

    return result;
}

int main(int argc, char *argv[]) {
    int server_port = 5555;
    int fill = 0;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"port", required_argument, 0, 'p'},
            {"fill", required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-hp:f:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'h':
                std::cout 
                    << "Redis clone client. Usage: ./client [OPTIONS]\n"
                    << "\n"
                    << "Connects to a leader node on the specified port.\n"
                    << "Requests are made to stdin, and responses are sent to stdout.\n"
                    << "\n"
                    << "-h, --help: displays this menu!\n"
                    << "-p, --port: port used by server for client connections. Default is 5555\n"
                    << "-f, --fill: prefill the cache with random keys and values. Default is 0"
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
            case 'f':
                if (!optarg) {
                    std::cout << "Must enter a number!" << std::endl;
                    return EXIT_FAILURE;
                }
                fill = atoi(optarg);
                if (fill < 0) {
                    std::cout << "Fill must be non negative!" << std::endl;
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
        bool got_reply = false;

        std::cout << "> ";
        std::flush(std::cout);

        std::string input;

        if (fill > 0) {
            input = "set " + randomStr() + " " + randomStr() + "\n";
            fill--;
        } else {
            std::getline(std::cin, input);
        }
    
        socket.send(zmq::buffer(input), zmq::send_flags::none);
        
        while (!got_reply) {
            try {
                zmq::message_t reply;
                zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
                if (!res.has_value()) {
                    throw std::strerror(errno);
                }
                got_reply = true;
                std::cout << reply.to_string() << std::endl;
            } catch (...) {
                std::cout << "Error communicating with server: " << std::strerror(errno) << std::endl;
            }
        }
        
    }

    return EXIT_SUCCESS;
}
