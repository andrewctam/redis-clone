#ifndef HASHMAP_H
#define HASHMAP_H

#define DEFAULT_INITIAL_CAPACITY 100
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

    int capacity;
    int size = 0;
    float loadFactor = LOAD_FACTOR;

    int hash_function(const std::string& key, int elements);
    void rehash();

public:
    HashMap();
    HashMap(int);

    void add(const std::string& key, V* value);
    V* get(const std::string& key);
    bool remove(const std::string& key);

    int get_capacity();
    int get_size();
};

#endif