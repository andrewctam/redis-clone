#include "store.h"
#include "server.h"

using namespace server;
using namespace store;

int main() {
    start_server();
    start_store();
}