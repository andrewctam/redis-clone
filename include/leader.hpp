#ifndef LEADER_H
#define LEADER_H

#include <zmq.hpp>
#include "consistent-hashing.hpp"

void start_leader();
void handle_internal_requests();
void handle_client_requests();

#endif