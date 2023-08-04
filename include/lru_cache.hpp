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
    long size() { return keyMap.size(); }

    // adds an entry, deleting the entry at the start of the LRU queue if full
    void add(const std::string& key, BaseEntry *value);
    BaseEntry *remove(const std::string& key);

    BaseEntry *get(const std::string& key);
    CacheEntry *get_cache_entry(const std::string& key);

    std::vector<std::string> key_set(bool single_str = false);

    bool set_expire(const std::string& key, std::time_t time);

    void clear();

    // returns import_strs for all entries grouped based on the specified bounds: [prev_bound, cur_bound)
    // for example: given [50, 100, 200] and start = 0, partitions the cache into hashes of values [0, 50), [50, 100), [100, 200). Values >= 200 are ignored. 
    // If start is negative, it will wrap around. e.g. start = -300, then that first bound would be [300, 360) U [0, 50).
    // also moves all grouped entries to the start of the LRU queue for imminent deletion
    std::vector<std::string> extract(int start, std::vector<int> upper_bounds);

    // import entries from another LRU cache from extract()
    bool import(const std::string& import_str);
};

#endif
