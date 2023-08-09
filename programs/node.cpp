#include <getopt.h>

#include "leader.hpp"
#include "worker.hpp"
#include "globals.hpp"
#include "consistent-hashing.hpp"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;
int ms_offset = 0;
int client_port = 5555;
int internal_port = -1;
ConsistentHashing ring;

int main(int argc, char *argv[]) {
    bool leader = false;
    bool worker = false;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"client-port", required_argument, 0, 'c'},
            {"internal-port", required_argument, 0, 'i'},
            {"worker", no_argument, 0, 'w'},
            {"leader", no_argument, 0, 'l'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-hc:i:wl", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'h':
                std::cout 
                    << "Redis clone node. Usage: ./node [OPTIONS]\n"
                    << "\n"
                    << "Creates a worker or leader node connected to the internal port. If -w or -l is not provided, the node will default to a worker node.\n"
                    << "Worker nodes must be created after a leader node using the same internal port.\n"
                    << "Worker nodes can be upgraded to a leader node if connection is lost to the leader\n"
                    << "\n"
                    << "-h, --help: displays this menu!\n"
                    << "-w, --worker: flag to specify a node is a worker node.\n"
                    << "-l, --leader: flag to specify a node is a leader node.\n"
                    << "-c, --client-port: port used for client connections. Default is 5555\n"
                    << "-i, --internal-port: port used by nodes for internal communication. Default is the client port + 10000"
                    << std::endl;

                return EXIT_SUCCESS;
            case 'c':
                if (!optarg) {
                    std::cout << "Must enter a value" << std::endl;
                    return EXIT_FAILURE;
                }
                client_port = atoi(optarg);
                if (client_port <= 0) {
                    std::cout << "Client port must be greater than 0!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            case 'i':
                if (!optarg) {
                    std::cout << "Must enter a value" << std::endl;
                    return EXIT_FAILURE;
                }
                internal_port = atoi(optarg);
                if (internal_port <= 0) {
                    std::cout << "Internal port must be greater than 0!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            case 'w':
                if (leader) {
                    std::cout << "Can not be both a leader and worker!" << std::endl;
                    return EXIT_FAILURE;
                }

                worker = true;
                break;
            case 'l':
                if (worker) {
                    std::cout << "Can not be both a leader and worker!" << std::endl;
                    return EXIT_FAILURE;
                }

                leader = true;
                break;
            default:
                return EXIT_FAILURE;
        }
    }

    if (internal_port == -1) {
        internal_port = client_port + 10000;
    }

    if (leader) {
        start_leader();
    } else {
        start_worker();
    } 

    return EXIT_SUCCESS;
}
