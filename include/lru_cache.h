#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <iostream>
#include <vector>
#include <list>
#include <string>

#include "base_entry.h"

constexpr int DEFAULT_INITIAL_CAPACITY = 100;
constexpr double LOAD_FACTOR = 0.75;

class HashEntry {
public:
    std::string key;
    BaseEntry *value;

    HashEntry(const std::string& key, BaseEntry *value): key(key), value(value) { }

    ~HashEntry() {}
};

class LRUCache {
private:
    std::vector<
        std::list<
            HashEntry
        >
    > buckets;

    int size = 0;
    float loadFactor = LOAD_FACTOR;

    int hash_function(const std::string& key, int elements);
    void rehash();

public:
    LRUCache(int initial_size = DEFAULT_INITIAL_CAPACITY);
    int get_capacity();
    int get_size();

    void add(const std::string& key, BaseEntry *value);
    BaseEntry *get(const std::string& key);
    bool remove(const std::string& key);
    std::vector<std::string> key_set();
    bool set_expire(const std::string& key, std::time_t time);
};

#endif