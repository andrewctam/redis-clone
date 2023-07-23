#include "linked_list.h"

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

bool LinkedList::add_end(BaseEntry *value) {
    if (!value) {
        return false;
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
    return true;
}

bool LinkedList::add_front(BaseEntry *value) {
    if (!value) {
        return false;
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
    return true;
}

bool LinkedList::remove_end(bool del) {
    if (size == 0) {
        return false;
    }

    if (size == 1) {
        if (del) {
            delete tail;
        }
        head = nullptr;
        tail = nullptr;
        size--;

        return true;
    }

    Node *cur = tail;

    tail = tail->prev;
    tail->next = nullptr;
    if (del) {
        delete cur;
    }
    size--;

    return true;
}


bool LinkedList::remove_front(bool del) {
    if (size == 0) {
        return false;
    }

    if (size == 1) {
        delete head;
        head = nullptr;
        tail = nullptr;
        size--;

        return true;
    }

    Node *cur = head;

    head = head->next;
    head->prev = nullptr;
    if (del) {
        delete cur;
    }
    size--;

    return true;
}

bool LinkedList::remove(int i, bool del) {
    Node * node = get_node(i);
    if (!node) {
        return false;
    }

    return remove_node(node, del);
}

bool LinkedList::remove_node(Node *node, bool del) {
    if (!node || size == 0) {
        return false;
    }

    if (node == head) {
        return remove_front(del);
    } else if (node == tail) {
        return remove_end(del);
    }

    Node *next = node->next;
    Node *prev = node->prev;

    prev->next = next;
    next->prev = prev;
    size--;

    if (del) {
        delete node;
    }

    return true;
}

std::string LinkedList::values() {
    std::stringstream ss;
    ss << "[";

    Node *cur = head;
    while (cur) {
        if (cur != head) {
            ss << " ";
        }
        BaseEntry *value = cur->value;

        switch (value->get_type()) {
             case ValueType::integer:
                ss << std::to_string(dynamic_cast<IntEntry *>(value)->value);
                break;
            case ValueType::str:
                ss << dynamic_cast<StringEntry *>(value)->value;
                break;
            default:
                ss << "?";
        }

        cur = cur->next;

    }

    ss << "]";

    return ss.str();
}
