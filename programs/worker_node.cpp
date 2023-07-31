#include "worker.h"
#include "globals.h"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;

int main() {
    start_worker();
    return EXIT_SUCCESS;
}
