#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <sstream>
#include <iostream>
#include <vector>

#include "entries/base_entry.h"

class Node {
public:
    Node *next = nullptr;
    Node *prev = nullptr;

    //user is responsible for deleting this
    BaseEntry *value;

    Node(BaseEntry *value): value(value) {}
    ~Node() {}
};

class LinkedList {
private:
    int size = 0;
    Node *head = nullptr;
    Node *tail = nullptr;

public:
    int get_size();

    LinkedList() {}
    ~LinkedList() {}

    Node *get_node(int i);
    BaseEntry *get(int i);

    Node *add_end(BaseEntry *value);

    Node *add_front(BaseEntry *value);

    
    // deletes the nodes and returns the BaseEntry 
    // user should free this BaseEntry after use
    BaseEntry *remove_end();
    BaseEntry *remove_front();
    BaseEntry *remove(int i);
    // node will be freed and should not be used again
    BaseEntry *remove_node(Node *node);

    // returns elements between start and stop inclusive. 
    // if stop < 0, goes to the end of the list
    // if reverse is true, will traverse the list in reverse
    // if single_str is true, will return one string at index 0 representing the values
    std::vector<std::string> values(int start = 0, int stop = -1,
        bool reverse = false, bool single_str = false);
};


#endif