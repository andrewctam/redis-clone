#ifndef LIST_ENTRY_H
#define LIST_ENTRY_H

#include "base_entry.h"
#include "linked_list.h"

class ListEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::list; }
    LinkedList *list;

    std::string to_string() { 
        std::vector<std::string> str = list->values(0, -1, false, true);
        if (str.size() == 0) {
            return "[]";
        }

        return str[0];
    }
    
    ListEntry() {
        list = new LinkedList();
    }
    
    ~ListEntry() {
        delete list;
    }
};

#endif