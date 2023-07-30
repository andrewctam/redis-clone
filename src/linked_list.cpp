#include <sstream>

#include "linked_list.h"
#include "entries/cache_entry.h"

int LinkedList::get_size() {
    return size;
}

Node *LinkedList::get_node(int i) {
    if (i > size) {
        return nullptr;
    }

    if (i > size / 2) {
        i = size - 1 - i;

        Node *cur = tail;

        while (i > 0) {
            cur = cur->prev;
            i--;
        }

        return cur;
    } else {
        Node *cur = head;

        while (i > 0) {
            cur = cur->next;
            i--;
        }

        return cur;
    }
}


BaseEntry *LinkedList::get(int i) {
    Node *node = get_node(i);
    if (node) {
        return node->value;
    } else {
        return nullptr;
    }
}

Node *LinkedList::add_end(BaseEntry *value) {
    if (!value) {
        return nullptr;
    }

    Node *node = new Node(value);

    if (size == 0) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        node->prev = tail;

        tail = node;
    }

    size++;
    return node;
}

Node *LinkedList::add_front(BaseEntry *value) {
    if (!value) {
        return nullptr;
    }

    Node *node = new Node(value);

    if (size == 0) {
        head = node;
        tail = node;
    } else {
        head->prev = node;
        node->next = head;

        head = node;
    }

    size++;
    return node;
}

BaseEntry *LinkedList::remove_end() {
    if (size == 0) {
        return nullptr;
    }

    Node *cur = tail;
    if (size == 1) {
        head = nullptr;
        tail = nullptr;
    } else {
        tail = tail->prev;
        tail->next = nullptr;
    }

    size--;
    BaseEntry *val = cur->value;
    delete cur;
    
    return val;
}


BaseEntry *LinkedList::remove_front() {
    if (size == 0) {
        return nullptr;
    }

    Node *cur = head;
    if (size == 1) {
        head = nullptr;
        tail = nullptr;
    } else {
        head = head->next;
        head->prev = nullptr;
    }

    size--;
    BaseEntry *val = cur->value;
    delete cur;

    return val;
}

BaseEntry *LinkedList::remove(int i) {
    Node *node = get_node(i);
    if (!node) {
        return nullptr;
    }

    return remove_node(node);
}

BaseEntry *LinkedList::remove_node(Node *node) {
    if (!node || size == 0) {
        return nullptr;
    }

    if (node == head) {
        return remove_front();
    } else if (node == tail) {
        return remove_end();
    }

    Node *next = node->next;
    Node *prev = node->prev;

    prev->next = next;
    next->prev = prev;
    size--;
    BaseEntry *val = node->value;

    delete node;

    return val;
}

std::vector<std::string> LinkedList::values(int start, int stop, bool reverse, bool single_str) {
    if (start < 0) {
        start = 0;
    }

    if (stop < 0 || stop >= get_size()) {
        stop = get_size() - 1;
    }

    int i = 0;

    Node *cur = head;
    if (reverse) {
        cur = tail;
    }

    std::stringstream ss;
    std::vector<std::string> vals;
    vals.reserve(stop - start + 1);

    while (cur && i < start) {
        i++;
        if (reverse) {
            cur = cur->prev;
        } else {
            cur = cur->next;
        }
    }
    bool first_element = true;
    while (cur && i <= stop) {
        BaseEntry *value = cur->value;

        //for cache entries, filter out expired
        if ((value->get_type() != EntryType::cache) || 
            !(dynamic_cast<CacheEntry *>(value)->expired())) {
                
            std::string str = value->to_string();
            if (single_str) {
                if (!first_element) {
                    ss << " ";
                } else {
                    first_element = false;
                }
                
                ss << str;
            } else {
                vals.emplace_back(str);
            }
        }
        
        if (reverse) {
            cur = cur->prev;
        } else {
            cur = cur->next;
        }

        i++;
    }

    if (single_str) {
        return { ss.str() };
    } else {
        return vals;
    }
}

void LinkedList::clear() {
    Node *cur = head;

    while (cur) {
        Node *next = cur->next;
        delete cur->value;
        delete cur;
        cur = next;
    }

    size = 0;
    head = nullptr;
    tail = nullptr;
}
