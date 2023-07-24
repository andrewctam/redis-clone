#include "lru_cache.h"
#include "server.h"

LRUCache::LRUCache(int inital_size, int max_map_size): max_size(max_map_size) {
    if (max_map_size > keyMap.max_size()) {
        max_size = keyMap.max_size();
    }
    keyMap.reserve(inital_size);
}

void LRUCache::add(const std::string& key, BaseEntry *value) {
    CacheEntry *existing = get_cache_entry(key);

    if (existing) {
        if (existing->get_type() == EntryType::cache) {
            existing->cached = value;
        }
        return;
    }

    if (size() >= max_size) {
        BaseEntry *lru = entries.remove_front(true);

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

CacheEntry *LRUCache::get_cache_entry(const std::string& key) {
    auto it = keyMap.find(key);

    if (it == keyMap.end()) {
        return nullptr;
    }

    Node *node = it->second;

    //bring node to front of list
    BaseEntry *entry = entries.remove_node(node, false);
    entries.add_end(node);
    
    if (entry->get_type() == EntryType::cache) {
        CacheEntry * cache_entry = dynamic_cast<CacheEntry*>(entry);

        if (cache_entry->expired()) {
            keyMap.erase(it);
            entries.remove_node(node, true);

            return nullptr;
        } else {
            return cache_entry;
        }
    }

    return nullptr;
}

bool LRUCache::remove(const std::string& key) {
    auto it = keyMap.find(key);

    if (it == keyMap.end()) {
        return false;
    }
    
    Node *node = it->second;
    keyMap.erase(it);

    entries.remove_node(node, true);
    return true;
}


bool LRUCache::set_expire(const std::string& key, std::time_t time) {
    CacheEntry *cache_entry = get_cache_entry(key);
    if (cache_entry) {
        cache_entry->expiration = time;
        return true;
    }

    return false;
}

