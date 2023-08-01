#include <sstream>
#include <iterator>

#include "entries/base_entry.hpp"
#include "entries/list_entry.hpp"
#include "linked_list.hpp"

BaseEntry *str_to_base_entry(std::string str) {
    if (str.find(" ") != std::string::npos) {
        ListEntry *list_entry = new ListEntry();
        LinkedList *list = list_entry->list;

        std::istringstream iss(str);
        std::istream_iterator<std::string> it(iss);

        while (it != std::istream_iterator<std::string>()) {
            list->add_end(str_to_base_entry(*it));
            it++;
        }

        return list_entry;
    }

    try {
        for(char ch : str) {
            if (ch < '0' || ch > '9') {
                throw "Not an int";
            }
        }
        
        int intVal = std::stoi(str);
        return new IntEntry(intVal);
    } catch(...) {
        return new StringEntry(str);
    }
}
