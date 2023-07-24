#ifndef BASE_ENTRY_H
#define BASE_ENTRY_H

#include <string>
#include "unix_times.h"
#include <iostream>

enum class EntryType {
    none,
    cache,
    str,
    integer
};

class BaseEntry {
public:
    virtual EntryType get_type() { return EntryType::none; }

    virtual ~BaseEntry() { }
};

class CacheEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::cache; }
    std::string key;
    BaseEntry *cached;
    seconds::rep expiration = 0; //0 = won't expire

    bool expired() {
        return (expiration > 0 && expiration <= time_secs());
    }

    CacheEntry(std::string key, BaseEntry *cached): key(key), cached(cached) {}
    ~CacheEntry() {
        delete cached;
        std::cout << "Deleted" << std::endl;
    }
};

class StringEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::str; }
    std::string value;

    StringEntry(std::string value): value(value) {}
};

class IntEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::integer; }
    int value;

    IntEntry(int value): value(value) {}
};

#endif
