#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <getopt.h>

int main(int argc, char *argv[]) {
    int client_port = 5555;
    int internal_port = -1;
    int nodes = 4;
    int fill = 0;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"client-port", required_argument, 0, 'c'},
            {"internal-port", required_argument, 0, 'i'},
            {"nodes", required_argument, 0, 'n'},
            {"fill", required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-hc:i:n:f:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'h':
                std::cout 
                    << "Redis clone server. Usage: ./server [OPTIONS]\n"
                    << "\n"
                    << "Creates the specified number of nodes and a client.\n"
                    << "If this server is terminated, all the nodes will also be terminated for quick clean up.\n"
                    << "\n"
                    << "-h, --help: displays this menu!\n"
                    << "-c, --client-port: port used for client connections. Default is 5555\n"
                    << "-i, --internal-port: port used by nodes for internal communication. Default is the client port + 10000\n"
                    << "-n, --nodes: number of nodes (n - 1 workers, 1 leader). Default is 4\n"
                    << "-f, --fill: prefill the cache with random keys and values. Default is 0"
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
            case 'n':
                if (!optarg) {
                    std::cout << "Must enter a value" << std::endl;
                    return EXIT_FAILURE;
                }
                nodes = atoi(optarg);
                if (nodes <= 0) {
                    std::cout << "Nodes must be greater than 0!" << std::endl;
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
                    std::cout << "Number must be non negative!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;   
            default:
                std::cout << "Invalid option" << std::endl;
                return EXIT_FAILURE;
        }
    }

    if (internal_port == -1) {
        internal_port = client_port + 10000;
    }

    int ppid = getpid();
    char i_port[100], c_port[100];
    snprintf(i_port, sizeof(i_port), "%d", internal_port);
    snprintf(c_port, sizeof(c_port), "%d", client_port);

    
    // leader node
    int pid = fork();
    if (pid == 0) { //child
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1 || getppid() != ppid) {
            exit(EXIT_FAILURE);
        }

        execlp("./node", "./node", "-l", "-i", i_port, "-c", c_port, nullptr);

        std::cout << "Failed to start leader node. Make sure you are in ./build/programs before executing ./server" << std::endl;
        return EXIT_FAILURE;
    }

    // n - 1 worker nodes
    for (int i = 0; i < nodes - 1; i++) {
        int pid = fork();
        if (pid == 0) { //child
            if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1 || getppid() != ppid) {
                exit(EXIT_FAILURE);
            }

            execlp("./node", "./node", "-w", "-i", i_port, "-c", c_port, nullptr);
            
            std::cout << "Failed to start worker node. Make sure you are in ./build/programs before executing ./server" << std::endl;
            return EXIT_FAILURE;
        } 
    }

    usleep(1000 * 200);

    char fill_num[100];
    snprintf(fill_num, sizeof(fill_num), "%d", fill);

    //start a client
    execlp("./client", "./client", "-p", c_port, "-f", fill_num,  nullptr);

    std::cout << "Failed to start client. Make sure you are in ./build/programs before executing ./server" << std::endl;
    return EXIT_FAILURE;
} 