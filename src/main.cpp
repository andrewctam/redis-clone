#include "server.h"
#include "linked_list.h"

int main() {
      LinkedList list;

    list.add_end(new StringEntry("1"));
    list.add_end(new StringEntry("2"));
    list.add_end(new StringEntry("3"));

    list.add_front(new StringEntry("0"));
    
    std::string a = list.values();

    start_server();
    return EXIT_SUCCESS;
}