#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h> 
#include <errno.h>
#include <cstring>

#include <math.h>


#include "store.h"
#include "redis.h"

#define MAX_CLIENTS 20
#define PORT 9999

using namespace std;

int main() {
    store::load_data_from_disk();

    fd_set readfds;
    int client_sockets[MAX_CLIENTS];
    std::memset(client_sockets, 0, sizeof(client_sockets));



    int main_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_fd < 0) {
        cerr << "Failed to create socket fd" << endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(main_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0 ) {
        cerr << "Failed to bind" << endl;
        exit(EXIT_FAILURE);  
    }  

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(PORT);

    if (bind(main_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0) {
        cerr << "Failed to bind" << endl;
        exit(EXIT_FAILURE);
    }

    

    if (listen(main_fd, MAX_CLIENTS) < 0) {
        cerr << "Error while listening" << endl;
        exit(EXIT_FAILURE);
    }
    auto addrlen = sizeof(sockaddr);

    cerr << "Server started on Port " << PORT << endl;

    while (true) {
        FD_ZERO(&readfds);  
        FD_SET(main_fd, &readfds);  
        int max_sd = main_fd;
             
        for (int i = 0; i < MAX_CLIENTS; i++) {
            // add sd to the set
            int sd = client_sockets[i];   
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            // find max sd
            if (sd > max_sd) {
                max_sd = sd; 
            } 
        }  
     
        //wait for an activity on one of the sockets 
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);  
        if (activity < 0 && errno != EINTR) { 
            cerr << "Select error";
        }  
             
        //main socket activity
        if (FD_ISSET(main_fd, &readfds)) {
            int client = accept(main_fd, (struct sockaddr *) &sockaddr, (socklen_t*) &addrlen);
            if (client < 0) {
                cerr << "Accept error";
                exit(EXIT_FAILURE);
            }  
             
            cerr << "New client connection with sd " << client;  
        
            //add connfd to clients 
            for (int i = 0; i < MAX_CLIENTS; i++) {  
                if (!client_sockets[i]) {
                    client_sockets[i] = client;
                    break;
                }
            }  
        }  
             
        //client activity
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];  
                 
            if (FD_ISSET(sd, &readfds)) {
                //Check if it was for closing , and also read the 
                //incoming message 
                char buf[1024];
                int num_read = read(sd , buf, 1024);
                if (num_read == 0) {
                    //disconnected, remove from clients 
                    close(sd); 
                    client_sockets[i] = 0;  
                } else {  
                    buf[num_read] = '\0';  
                    send(sd, buf , strlen(buf) , 0 );  
                }  
            }  
        }  
    }

    close(main_fd);
    return EXIT_SUCCESS;
}