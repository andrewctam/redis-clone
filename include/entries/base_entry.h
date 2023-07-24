#ifndef BASE_ENTRY_H
#define BASE_ENTRY_H

#include <string>
#include <iostream>

#include "unix_times.h"

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

    virtual ~BaseEntry() { }
};


// entries constructed of primitives

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
