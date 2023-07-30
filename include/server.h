#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <errno.h>
#include <math.h>

#include <cstring>
#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "lru_cache.h"

constexpr int CLIENT_PORT = 9001;
constexpr int LEADER_PORT = CLIENT_PORT + 10000;

extern bool monitoring;
extern bool stop;

extern LRUCache cache;

void start_leader();
void start_worker();
#endif