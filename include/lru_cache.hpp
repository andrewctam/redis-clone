#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <vector>
#include <unordered_map>
#include <string>

#include "entries/base_entry.hpp"
#include "entries/cache_entry.hpp"
#include "linked_list.hpp"

constexpr long DEFAULT_INITIAL_SIZE = 100;
constexpr long DEFAULT_MAX_SIZE = 5000;

class LRUCache {
private:
    std::unordered_map<std::string, Node*> keyMap;

    //LinkedList of CacheEntry
    LinkedList entries; 
public:
    LRUCache(long initial_size = DEFAULT_INITIAL_SIZE, long max_map_size = DEFAULT_MAX_SIZE);
    LRUCache(const std::string &import_str, long initial_size = DEFAULT_INITIAL_SIZE, long max_map_size = DEFAULT_MAX_SIZE);

    ~LRUCache() { clear(); }
    
    long max_size;
    long size() { 
        return keyMap.size();
    }

    void add(const std::string& key, BaseEntry *value);
    BaseEntry *remove(const std::string& key);

    BaseEntry *get(const std::string& key);
    CacheEntry *get_cache_entry(const std::string& key);

    std::vector<std::string> key_set(bool single_str = false);

    bool set_expire(const std::string& key, std::time_t time);

    void clear();

    // removes all entries with a hash value less than upper_bound, while
    // returning a string that can be sent to another LRU cache
    // to add these elements using import()
    std::string extract(int upper_bound);
    bool import(const std::string& import_str);
};

#endif