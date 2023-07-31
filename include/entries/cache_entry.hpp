#ifndef CACHE_ENTRY_H
#define CACHE_ENTRY_H

#include "base_entry.hpp"

class CacheEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::cache; }
    std::string key;
    BaseEntry *cached;
    seconds::rep expiration = 0; //0 = won't expire

    bool expired() {
        return (expiration > 0 && expiration <= time_secs());
    }

    std::string to_string() { return key; }

    CacheEntry(std::string key, BaseEntry *cached): key(key), cached(cached) {}
    ~CacheEntry() { }
};

#endif