#include <list>
#include <iostream>

#include "entries/base_entry.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "consistent-hashing.hpp"


LRUCache::LRUCache(long inital_size, long max_map_size): max_size(max_map_size) {
    if (max_map_size > keyMap.max_size()) {
        max_size = keyMap.max_size();
    }
    keyMap.reserve(inital_size);
}

LRUCache::LRUCache(const std::string &import_str, long inital_size, long max_map_size): LRUCache(inital_size, max_map_size) {
    import(import_str);
}



CacheEntry *LRUCache::get_cache_entry(const std::string& key) {
    auto it = keyMap.find(key);

    if (it == keyMap.end()) {
        return nullptr;
    }

    Node *node = it->second;

    //bring node to front of list
    BaseEntry *entry = entries.remove_node(node);
    it->second = entries.add_end(entry);
    
    if (entry->get_type() == EntryType::cache) {
        CacheEntry *cache_entry = dynamic_cast<CacheEntry*>(entry);

        if (cache_entry->expired()) {
            delete entries.remove_node(it->second);
            keyMap.erase(it);

            return nullptr;
        } else {
            return cache_entry;
        }
    }

    return nullptr;
}

void LRUCache::add(const std::string& key, BaseEntry *value) {
    CacheEntry *existing = get_cache_entry(key);

    if (existing) {
        if (existing->get_type() == EntryType::cache) {
            delete existing->cached;
            existing->cached = value;
        }
        return;
    }

    if (size() >= max_size) {
        BaseEntry *lru = entries.remove_front();

        if (lru->get_type() == EntryType::cache) {
            CacheEntry *old_entry = dynamic_cast<CacheEntry*>(lru);
            keyMap.erase(old_entry->key);
        }

        delete lru;
    }

    CacheEntry *entry = new CacheEntry(key, value);
    Node *node = entries.add_end(entry);
    keyMap.insert({key, node});
}

BaseEntry *LRUCache::get(const std::string& key) {
    CacheEntry *cache_entry = get_cache_entry(key);

    if (cache_entry) {
        return cache_entry->cached;
    } else {
        return nullptr;
    }
}

BaseEntry *LRUCache::remove(const std::string& key) {
    auto it = keyMap.find(key);

    if (it == keyMap.end()) {
        return nullptr;
    }
    
    Node *node = it->second;
    keyMap.erase(it);

    BaseEntry *entry = entries.remove_node(node);
    if (entry->get_type() == EntryType::cache) {
        CacheEntry *cache_entry = dynamic_cast<CacheEntry*>(entry);
        BaseEntry *value = cache_entry->cached;

        delete cache_entry;
        return value;
    }

    return nullptr;
}

std::vector<std::string> LRUCache::key_set(bool single_str) {
    if (single_str) {
        return entries.values(0, -1, false, true);    
    } else {
        return entries.values();
    }
}

bool LRUCache::set_expire(const std::string& key, std::time_t time) {
    CacheEntry *cache_entry = get_cache_entry(key);
    if (cache_entry) {
        if (time == 0 && cache_entry->expiration == 0) {
            //already 0
            return false;
        }

        cache_entry->expiration = time;
        return true;
    }

    return false;
}

void LRUCache::clear() {
    entries.clear();
    keyMap.clear();
}

bool in_range(int val, int low, int high) {
    if (high < low) { //wrap around
        return low <= val || val < high; 
    } else {
        return low <= val && val < high;
    }
}

std::vector<std::string> LRUCache::extract(std::vector<int> upper_bounds) {
    if (upper_bounds.size() < 2) {
        return {};
    }

    std::vector<std::stringstream *> import_strs;
    int n = upper_bounds.size();
    for (int i = 0; i < n - 1; i++) {
        import_strs.emplace_back( new std::stringstream() );
    }

    for (auto it = keyMap.begin(); it != keyMap.end(); it++) {
        std::string key = it->first;
        int hash = hash_function(key);

        for (int i = 0; i < n - 1; i++) {
            if (in_range(hash, upper_bounds[i], upper_bounds[i + 1])) {
                // add this key/value to the string
                BaseEntry *entry = it->second->value;
                if (entry->get_type() == EntryType::cache) {
                    std::stringstream *ss = import_strs[i];
                    
                    CacheEntry *cache_entry = dynamic_cast<CacheEntry*>(entry);
                    *ss << key << "\n" << cache_entry->cached->to_string() << "\n";
                }

                //move node for LRU deletion
                Node *node = it->second;
                it->second = entries.add_front(entries.remove_node(node));
                break;
            }
        }

    }
   
    std::vector<std::string> strs;
    for (auto &ss : import_strs) {
        std::string str = ss->str();
        strs.emplace_back(str);
        
        delete ss;
    }

    return strs;
}

bool LRUCache::import(const std::string& import_str) {
    std::istringstream iss(import_str);

    for (std::string key; std::getline(iss, key); ) {
        std::string value;
        if (std::getline(iss, value)) {
            add(key, str_to_base_entry(value));
        } else {
            return false;
        }
    }

    return true;
}
