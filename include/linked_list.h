#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "base_entry.h"
#include <sstream>
#include <iostream>
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

    bool add_end(BaseEntry *value);
    bool add_front(BaseEntry *value);

    bool remove_end(bool del = true);
    bool remove_front(bool del = true);
    bool remove(int i, bool del = true);
    bool remove_node(Node *node, bool del = true);

    std::string values();

};


#endif