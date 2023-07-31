#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    int nodes = 4;

    if (argc > 1) {
        nodes = atoi(argv[1]);

        if (nodes <= 1) {
            std::cout << "Invalid number of nodes" << std::endl;
            return EXIT_FAILURE;
        }
    }
    // if this server dies, kill the nodes
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    for (int i = 0; i < nodes - 1; i++) {
        int pid = fork();
        if (pid == 0) { //child
            execlp("./worker_node", "./worker_node", nullptr);
            return EXIT_FAILURE;
        } 
    }

    int pid = fork();
    if (pid == 0) { //child
        execlp("./leader_node", "./leader_node", nullptr);
        std::cout << "Failed to start leader node. Make sure you are in ./build/programs before executing ./server" << std::endl;
        return EXIT_FAILURE;
    }

    sleep(1);

    //start a client
    execlp("./client", "./client", "5555", nullptr);
    return EXIT_FAILURE;
} 