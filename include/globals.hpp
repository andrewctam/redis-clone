#ifndef GLOBALS_H
#define GLOBALS_H

#include "lru_cache.hpp"
#include "consistent-hashing.hpp"

// global vars used by the nodes
extern int client_port;
extern int internal_port;
extern ConsistentHashing ring;

extern bool monitoring;
extern bool stop;
extern LRUCache cache;

#endif