#ifndef LEADER_H
#define LEADER_H

#include <zmq.hpp>

#include "consistent-hashing.hpp"

void start_leader();
void handle_new_node(zmq::socket_t &internal_socket, zmq::socket_t &dealer_socket, ConsistentHashing &ring);
void handle_client_request(zmq::socket_t &client_socket, zmq::socket_t &dealer_socket, ConsistentHashing &ring);

#endif