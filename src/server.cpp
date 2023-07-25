#include "command.h"
#include "lru_cache.h"
#include "server.h"
#include "unix_times.h"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;

void start_server() {
    fd_set readfds;
    std::vector<int> client_sockets;

    int main_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (main_fd < 0) {
        std::cerr << "Failed to create socket fd" << std::endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(main_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0 ) {
        std::cerr << "Failed to bind" << std::endl;
        exit(EXIT_FAILURE);  
    }  

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(PORT);

    if (bind(main_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0) {
        std::cerr << "Failed to bind" << std::endl;
        exit(EXIT_FAILURE);
    }

    

    if (listen(main_fd, MAX_CLIENTS) < 0) {
        std::cerr << "Error while listening" << std::endl;
        exit(EXIT_FAILURE);
    }
    auto addrlen = sizeof(sockaddr);

    std::cerr << "Server started on Port " << PORT << std::endl;
    std::cout << "> ";
    std::flush(std::cout);

    while (true) {
        FD_ZERO(&readfds);  
        FD_SET(main_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        int max_sd = main_fd;
             
        for (int sd : client_sockets) {  
            FD_SET(sd, &readfds);
            
            // find max sd
            if (sd > max_sd) {
                max_sd = sd; 
            } 
        }  
     
        //wait for an activity on one of the sockets 
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);  
        if (activity < 0 && errno != EINTR) { 
            std::cerr << "Select error";
        }  
             
        //main socket activity
        if (FD_ISSET(main_fd, &readfds) && client_sockets.size() < MAX_CLIENTS) {
            int client = accept(main_fd, (struct sockaddr *) &sockaddr, (socklen_t*) &addrlen);
            if (client < 0) {
                std::cerr << "Accept error";
                exit(EXIT_FAILURE);
            }  
            
            client_sockets.push_back(client);
            std::cerr << "New client connection with sd " << client << 
                ". Total " << client_sockets.size() << " connected" << std::endl;
        } 

        //terminal command
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string input;
            std::getline(std::cin, input);

            Command cmd { input };
            std::cout << cmd.parse_cmd();
            
            std::cout << "> ";
            std::flush(std::cout);
        }
             
        //client activity
        for (int sd : client_sockets) {            
            if (FD_ISSET(sd, &readfds)) {
                //Check if it was for closing , and also read the 
                //incoming message 
                char buf[1024];
                int num_read = read(sd , buf, 1024);
                if (num_read == 0) {
                    close(sd); 
                    auto pos = std::find(client_sockets.begin(), client_sockets.end(), sd);
                    if (pos != client_sockets.end()) {
                        client_sockets.erase(pos);

                        std::cerr << "Client with sd " << sd << 
                        " disconnected. Total " << client_sockets.size() << " connected" << std::endl;
                    }

                    
                } else { 
                    buf[num_read] = '\0';
                    Command cmd { buf };
                    std::string res = cmd.parse_cmd();
                    send(sd, res.c_str(), res.length(), 0);  
                }  
            }  
        }

        if (stop) {
            break;
        }
    }

    //clean up
    for (int sd : client_sockets) {
        close(sd);
    }
    close(main_fd);
}

