#include "worker.hpp"
#include "globals.hpp"
#include <getopt.h>

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;
int client_port = 5555;
int internal_port = -1;

int main(int argc, char *argv[]) {
    LRUCache cache { 5, 5 };

    std::string str = 
    "key1\n"
    "value1\n"
    "key2\n"
    "2\n"
    "key3\n"
    "str 2 3\n";
    cache.import(str);

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"client-port", required_argument, 0, 'c'},
            {"internal-port", required_argument, 0, 'i'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-hc:i:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'h':
                std::cerr 
                    << "Redis clone worker node. Usage: ./worker_node [OPTIONS]\n"
                    << "-h, --help: displays this menu!\n"
                    << "-c, --client-port: port used for client connections. Will be used if this worker node is promoted to a leader. Default is 5555\n"
                    << "-i, --internal-port: port used by nodes for internal communication. Tries to connect to this port on startup. Default is the client port + 10000"
                    << std::endl;

                return EXIT_SUCCESS;
            case 'c':
                if (!optarg) {
                    std::cerr << "Must enter a value" << std::endl;
                    return EXIT_FAILURE;
                }
                client_port = atoi(optarg);
                if (client_port <= 0) {
                    std::cerr << "Client port must be greater than 0!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            case 'i':
                if (!optarg) {
                    std::cerr << "Must enter a value" << std::endl;
                    return EXIT_FAILURE;
                }
                internal_port = atoi(optarg);
                if (internal_port <= 0) {
                    std::cerr << "Internal port must be greater than 0!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            default:
                std::cerr << "Unknown option: " << optarg << std::endl;
                return EXIT_FAILURE;
        }
    }

    if (internal_port == -1) {
        internal_port = client_port + 10000;
    }

    start_worker();
    return EXIT_SUCCESS;
}
