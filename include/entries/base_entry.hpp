#ifndef BASE_ENTRY_H
#define BASE_ENTRY_H

#include <string>

#include "unix_times.hpp"

enum class EntryType {
    none,
    cache,
    str,
    integer,
    list
};

class BaseEntry {
public:
    virtual EntryType get_type() { return EntryType::none; }
    virtual std::string to_string() { return "?"; }

    virtual ~BaseEntry() { }
};

BaseEntry *str_to_base_entry(std::string str);

// entries constructed of primitives

class StringEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::str; }
    std::string value;

    std::string to_string() { return value; }
    StringEntry(std::string value): value(value) {}
};

class IntEntry: public BaseEntry {
public:
    EntryType get_type() { return EntryType::integer; }
    int value;
    
    std::string to_string() { return std::to_string(value); }
    IntEntry(int value): value(value) {}
};


#endif
