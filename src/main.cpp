#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>

#include "server.h"

int main(int argc, char *argv[]) {
    int nodes = 4;

    if (argc > 1) {
        nodes = atoi(argv[1]);

        if (nodes <= 1) {
            std::cout << "Invalid number of nodes" << std::endl;
            return EXIT_FAILURE;
        }
    }
    

    for (int i = 0; i < nodes - 1; i++) {
        int pid = fork();
        if (pid == 0) { //child
            prctl(PR_SET_PDEATHSIG, SIGHUP);
            start_worker();
            return EXIT_SUCCESS;
        } 
    }

    start_leader();
    
    return EXIT_SUCCESS;
} 