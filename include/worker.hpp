#ifndef WORKER_H
#define WORKER_H

#include <zmq.hpp>
#include "consistent-hashing.hpp"
#include <mutex>

extern std::string endpoint;
extern std::mutex endpoint_mutex;

void start_worker();

void handle_reqs();
void handle_pings();

void leader_election();

constexpr char COMMAND = '0';
constexpr char NODE_COMMAND = '1';
constexpr char RING_UPDATE= '2';
constexpr char CACHE_UPDATE = '3';
constexpr char ELECTION_MSG = '4';
constexpr char ELECTION_VICTORY = '5';

#endif