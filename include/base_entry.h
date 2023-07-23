#ifndef BASE_ENTRY_H
#define BASE_ENTRY_H

enum class ValueType {
    none,
    str 
};

class BaseEntry {
public:
    virtual ValueType get_type() { return ValueType::none; }
    ~BaseEntry() {}
};

class StringEntry: public BaseEntry {
public:
    ValueType get_type() { return ValueType::str; }
    std::string value;

    StringEntry(std::string value): value(value) {}
};

#endif
