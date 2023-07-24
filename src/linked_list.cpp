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

Node *LinkedList::add_end(BaseEntry *value) {
    if (!value) {
        return nullptr;
    }

    return add_end(new Node(value));
}

Node *LinkedList::add_end(Node *node) {
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

BaseEntry *LinkedList::remove_end(bool del) {
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
    if (del) {
        delete cur;
    } else {
        cur->prev = nullptr;
        cur->next = nullptr;
    }
    return val;
}


BaseEntry *LinkedList::remove_front(bool del) {
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
    if (del) {
        delete cur;
    } else {
        cur->prev = nullptr;
        cur->next = nullptr;
    }

    return val;
}

BaseEntry *LinkedList::remove(int i, bool del) {
    Node *node = get_node(i);
    if (!node) {
        return nullptr;
    }

    return remove_node(node, del);
}

BaseEntry *LinkedList::remove_node(Node *node, bool del) {
    if (!node || size == 0) {
        return nullptr;
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
    BaseEntry *val = node->value;

    if (del) {
        delete node;
    } else {
        node->next = nullptr;
        node->prev = nullptr;
    }

    return val;

}

std::vector<std::string> LinkedList::values() {
    std::vector<std::string> vals;
    vals.reserve(get_size());

    Node *cur = head;
    while (cur) {
        BaseEntry *value = cur->value;
        switch (value->get_type()) {
             case EntryType::integer:
                vals.emplace_back(
                    std::to_string(dynamic_cast<IntEntry *>(value)->value));
                break;
            case EntryType::str:
                vals.emplace_back(
                    dynamic_cast<StringEntry *>(value)->value);
                break;
            case EntryType::cache: {
                CacheEntry *cache_entry = dynamic_cast<CacheEntry *>(value);
                if (!cache_entry->expired()) {
                    vals.emplace_back(cache_entry->key);
                }
                break;
            }
            default:
                vals.emplace_back("?");
        }

        cur = cur->next;

    }

    return vals;
}

