#ifndef WORKER_H
#define WORKER_H

#include <zmq.hpp>
#include "consistent-hashing.hpp"
#include <mutex>

extern std::string endpoint;
extern std::mutex endpoint_mutex;

void start_worker();

void handle_leader_reqs();
void handle_pings();

void start_leader_election();
void promote_to_leader();

#endif