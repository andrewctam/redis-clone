#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <sstream>
#include <iostream>
#include <vector>

#include "base_entry.h"

class Node {
public:
    Node *next = nullptr;
    Node *prev = nullptr;

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
    Node *add_end(Node *value);

    Node *add_front(BaseEntry *value);

    // returns the removed node's value, or nullptr if failure. 
    // If del is true, will free the node's memory.
    BaseEntry *remove_end(bool del = true);
    BaseEntry *remove_front(bool del = true);
    BaseEntry *remove(int i, bool del = true);
    BaseEntry *remove_node(Node *node, bool del = true);

    std::vector<std::string> values();
};


#endif