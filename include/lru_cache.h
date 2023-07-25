#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <string>

#include "entries/base_entry.h"
#include "entries/cache_entry.h"

#include "linked_list.h"

constexpr long DEFAULT_INITIAL_SIZE = 100;
constexpr long DEFAULT_MAX_SIZE = 5000;

class LRUCache {
private:
    std::unordered_map<std::string, Node*> keyMap;

    //LinkedList of CacheEntry
    LinkedList entries; 

public:
    LRUCache(long initial_size = DEFAULT_INITIAL_SIZE, long max_map_size = DEFAULT_MAX_SIZE);
    ~LRUCache() { clear(); }
    
    long max_size;
    long size() { 
        return keyMap.size();
    }

    void add(const std::string& key, BaseEntry *value);
    bool remove(const std::string& key);

    BaseEntry *get(const std::string& key);
    CacheEntry *get_cache_entry(const std::string& key);

    std::vector<std::string> key_set(bool single_str = false) {
        if (single_str) {
            return entries.values(0, -1, false, true);    
        } else {
            return entries.values();
        }
    }

    bool set_expire(const std::string& key, std::time_t time);

    void clear();
};

#endif
