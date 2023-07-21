#ifndef HASHMAP_H
#define HASHMAP_H

#define INITAL_CAPACITTY 100
#define LOAD_FACTOR 0.75

#include <iostream>
#include <vector>
#include <list>
#include <string>

template<typename V>
class HashEntry {
public:
    std::string key;
    V *value;

    HashEntry(const std::string& key, V* value): key(key), value(value) {}
};

template<typename V>
class HashMap {
private:
    std::vector<
        std::list<
            HashEntry<V>
        >
    > buckets;

    int capacity = INITAL_CAPACITTY;
    int size = 0;
    float loadFactor = LOAD_FACTOR;

    int hash_function(const std::string& key);
    void rehash();

public:
    HashMap();

    void add(const std::string& key, V* value);
    V* get(const std::string& key);
    bool remove(const std::string& key);
};

#endif