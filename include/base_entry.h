#ifndef BASE_ENTRY_H
#define BASE_ENTRY_H

#include "unix_secs.h"

enum class ValueType {
    none,
    str 
};

class BaseEntry {
public:
    virtual ValueType get_type() { return ValueType::none; }
    seconds::rep expiration = 0; //0 = won't expire

    ~BaseEntry() {}
};

class StringEntry: public BaseEntry {
public:
    ValueType get_type() { return ValueType::str; }
    std::string value;

    StringEntry(std::string value): value(value) {}
};

#endif
