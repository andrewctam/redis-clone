#ifndef LIST_ENTRY_H
#define LIST_ENTRY_H

#include "base_entry.h"
#include "linked_list.h"

class ListEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::list; }
    LinkedList *list;

    ListEntry() {
        list = new LinkedList();
    }
    
    ~ListEntry() {
        delete list;
    }
};

#endif