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

#include "hashmap.h"

constexpr int MAX_CLIENTS = 20;
constexpr int PORT = 9001;

extern bool monitoring;
extern bool stop;

extern HashMap hashmap;

void start_server();

#endif