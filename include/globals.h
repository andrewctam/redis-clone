#ifndef GLOBALS_H
#define GLOBALS_H

#include "lru_cache.h"

constexpr int CLIENT_PORT = 5555;
constexpr int LEADER_PORT = CLIENT_PORT + 10000;

// global vars used by the nodes
extern bool monitoring;
extern bool stop;
extern LRUCache cache;

#endif