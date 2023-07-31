#include "leader.hpp"
#include "globals.hpp"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;

int main() {
    start_leader();
    return EXIT_SUCCESS;
}
