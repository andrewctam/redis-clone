#include <thread>
#include <iostream>
#include <zmq.hpp>
#include <errno.h>
#include <string>
#include <csignal>

#include "command.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "unix_times.hpp"
#include "consistent-hashing.hpp"
#include "worker.hpp"
#include "leader.hpp"

std::string endpoint;
std::mutex endpoint_mutex;

bool election_in_progress = false;
bool recv_victory = false;
bool promoted_to_leader = false;
milliseconds::rep election_timeout = 0;

static volatile std::sig_atomic_t got_SIGUSR1;
static void SIGUSR1_handler(int signo) {
    got_SIGUSR1 = 1;
}

void start_worker() {
    struct sigaction sig;
    sig.sa_handler = SIGUSR1_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    if (sigaction(SIGUSR1, &sig, NULL) ) {
        std::perror("sigaction");
        exit(EXIT_FAILURE);
    } 

    // lock this mutex, will be unlocked once the reqs thread sets endpoint
    endpoint_mutex.lock();
    std::thread reqs_thread(handle_reqs);
    
    // wait until the reqs thread sets the endpoint
    std::thread ping_thread(handle_pings);

    reqs_thread.join();
    ping_thread.join();

    if (promoted_to_leader) {
        start_leader();
    }
}


void handle_reqs() {
    std::string worker_pid = std::to_string(getpid());

    // open an endpoint
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.set(zmq::sockopt::linger, 0);
    socket.set(zmq::sockopt::rcvtimeo, 1000);
    socket.bind("tcp://*:0");

    // set endpoint and unlock mutex
    endpoint = socket.get(zmq::sockopt::last_endpoint);
    endpoint_mutex.unlock();

    // listen for requests
    while (true) {
        try {
            zmq::message_t request;
            zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
        
            std::string response = "";
            std::string msg = request.to_string();

            if (msg.size() > 0) {
                std::string msg_body = msg.substr(1);
                char type = msg.at(0);

                // leader msgs should end an election
                switch(type) {
                    case COMMAND:
                    case RING_UPDATE:
                    case ELECTION_VICTORY:
                        if (election_in_progress) {
                            recv_victory = true;
                        }
                        break;
                    default:
                    break;
                }

                // process msg
                switch(type) {
                    case COMMAND: {
                        Command cmd { msg_body };
                        response = cmd.parse_cmd();
                        break;
                    } 
                    case NODE_COMMAND:
                        if (cmd::nodeCmds(cmd::extract_name(msg_body)) == cmd::NodeCMDType::Kill && 
                            cmd::extract_key(msg_body) == worker_pid
                        ) {
                            socket.send(zmq::buffer("OK"), zmq::send_flags::none);
                            std::cout << "Stopping " << worker_pid << std::endl;
                            exit(EXIT_SUCCESS);
                        }
                        break;
                    case RING_UPDATE:
                        ring.update(msg_body);
                        response = std::to_string(ring.size());  
                        break;
                    case CACHE_UPDATE: {
                        int old_size = cache.size();
                        cache.import(msg_body);
                        response = std::to_string(cache.size() - old_size);
                        break;
                    }
                    case ELECTION_MSG:
                        if (!election_in_progress && !recv_victory && !promoted_to_leader) {
                            std::cout << worker_pid << " got ELECTION_MSG. Calling an election..." << std::endl;
                            election_in_progress = true;
                            std::thread election_thread(leader_election);
                            election_thread.detach();
                        }

                        response = worker_pid;
                        break;
                    case ELECTION_VICTORY:
                        response = "OK";
                        break;
                    default:
                        response = "Missing type char";
                }
            }
            socket.send(zmq::buffer(response), zmq::send_flags::none);
        } catch (...) {
        }
        
        if (stop) {
            std::cout << "Stopping worker node pid " << worker_pid << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (got_SIGUSR1) {
            return;
        }
    }
}

void handle_pings() {
    // wait until the reqs thread sets the endpoint
    endpoint_mutex.lock();
    endpoint_mutex.unlock();

    std::string worker_pid = std::to_string(getpid());
    std::string ping = worker_pid + " " + endpoint;
    std::string leader_endpoint = "tcp://localhost:" + std::to_string(internal_port);

    // open connection to leader to send endpoint of this
    zmq::context_t leader_context{1};
    zmq::socket_t leader_socket{leader_context, zmq::socket_type::req};
    leader_socket.set(zmq::sockopt::linger, 0);
    leader_socket.set(zmq::sockopt::rcvtimeo, 1000);

    leader_socket.connect(leader_endpoint);
            
    while (true) {        
        // send pid and endpoint to leader
        leader_socket.send(zmq::message_t(ping), zmq::send_flags::none);
        bool got_reply = false;
        while (!got_reply) {
            try {
                // verify the ping was successfully recieved
                zmq::message_t reply;
                zmq::recv_result_t res = leader_socket.recv(reply, zmq::recv_flags::none);
                if (!res.has_value()) {
                    throw std::strerror(errno);
                }
                ring.update(reply.to_string());
                got_reply = true;
                std::this_thread::sleep_for(200ms);
                
            } catch (...) {
                if (!election_in_progress && !promoted_to_leader) {
                    std::cout << worker_pid << " did not get ping response from leader. Calling an election." << std::endl;
                    election_in_progress = true;
                    std::thread election_thread(leader_election);
                    election_thread.detach();
                }
            }
        
            if (got_SIGUSR1) {
                return;
            }
        }

    }
}

void leader_election() {
    recv_victory = false;
    std::string pid = std::to_string(getpid());

    std::vector<ServerNode*> candidates = ring.election_candidates();

    // if this is empty, then all the below gets skipped and this becomes leader
    if (candidates.size() > 0) {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::dealer); 
        socket.set(zmq::sockopt::linger, 0);
        socket.set(zmq::sockopt::rcvtimeo, 200);

        for (auto cand : candidates) {
            socket.connect(cand->endpoint);
        }

        std::string election_notif = ELECTION_MSG + pid;
        for (auto cand : candidates) {
            socket.send(zmq::message_t(), zmq::send_flags::sndmore);
            socket.send(zmq::buffer(election_notif), zmq::send_flags::none);
        }


        try {
            zmq::message_t request;
            // empty envelope
            zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
            // actual result
            res = socket.recv(request, zmq::recv_flags::none);
            
            election_in_progress = false;
            std::cout << pid << " received a response. Ending election" << std::endl;
            return;
        } catch (...) {
            std::cout << pid << " did not recieve any responses!" << std::endl;
        }
    }

    // election may have ended via ELECTION_VICTORY in other thread
    if (recv_victory) {
        std::cout << pid << " received ELECTION_VICTORY. Ending election" << std::endl;
        recv_victory = false;
        election_in_progress = false;
        return;
    }
    
    // no responses, election was won
    std::cout << "Promoting " << pid << " to leader!" << std::endl;
    promoted_to_leader = true;
    ring.election_cleanup();
    ring.set_up_dealer();
    ring.dealer_send(ELECTION_VICTORY + pid);

    ring.mutex.lock();
    int count = 0;
    while (count < ring.dealer_connected) {
        try {
            zmq::message_t dealer_request;
            // empty envelope
            zmq::recv_result_t res = ring.dealer_socket->recv(dealer_request, zmq::recv_flags::none);
            // actual result
            res = ring.dealer_socket->recv(dealer_request, zmq::recv_flags::none);
            count++;
        } catch (...) {
            std::cout << "Recv " << count << " out of " << ring.size() << std::endl;
        }
    }
    ring.mutex.unlock();

    raise(SIGUSR1);
    election_in_progress = false;
}
