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

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"client-port", required_argument, 0, 'c'},
            {"internal-port", required_argument, 0, 'i'},
            {"nodes", required_argument, 0, 'n'},
            {0, 0, 0, 0}
        };
        int c = getopt_long(argc, argv, "-hc:i:n:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch(c) {
            case 'h':
                std::cerr 
                    << "Redis clone server. Usage: ./server [OPTIONS]\n"
                    << "-h, --help: displays this menu!\n"
                    << "-c, --client-port: port used for client connections. Default is 5555\n"
                    << "-i, --internal-port: port used by nodes for internal communication. Default is the client port + 10000\n"
                    << "-n, --nodes: number of nodes (n - 1 workers, 1 leader). Default is 4"
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
            case 'n':
                if (!optarg) {
                    std::cerr << "Must enter a value" << std::endl;
                    return EXIT_FAILURE;
                }
                nodes = atoi(optarg);
                if (nodes <= 0) {
                    std::cerr << "Nodes must be greater than 0!" << std::endl;
                    return EXIT_FAILURE;
                }
                break;
            default:
                std::cerr << "Invalid option" << std::endl;
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

    // n - 1 worker nodes
    for (int i = 0; i < nodes - 1; i++) {
        int pid = fork();
        if (pid == 0) { //child
            if (prctl(PR_SET_PDEATHSIG, SIGINT) == -1 || getppid() != ppid) {
                exit(EXIT_FAILURE);
            }

            execlp("./worker_node", "./worker_node", "-i", i_port, "-c", c_port, nullptr);
            
            std::cerr << "Failed to start worker node. Make sure you are in ./build/programs before executing ./server" << std::endl;
            return EXIT_FAILURE;
        } 
    }

 
    // leader node
    int pid = fork();
    if (pid == 0) { //child
        if (prctl(PR_SET_PDEATHSIG, SIGINT) == -1 || getppid() != ppid) {
            exit(EXIT_FAILURE);
        }

        execlp("./leader_node", "./leader_node", "-i", i_port, "-c", c_port, nullptr);

        std::cerr << "Failed to start leader node. Make sure you are in ./build/programs before executing ./server" << std::endl;
        return EXIT_FAILURE;
    }

    //start a client
    execlp("./client", "./client", "-p", c_port, nullptr);

    std::cerr << "Failed to start client. Make sure you are in ./build/programs before executing ./server" << std::endl;
    return EXIT_FAILURE;
} 