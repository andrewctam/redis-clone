#include "hashmap.h"


HashMap::HashMap(int inital_capacity): capacity(inital_capacity) {
    buckets.resize(inital_capacity);
}

int HashMap::hash_function(const std::string& key, int elements) {
    std::hash<std::string> hasher;
    return hasher(key) % elements;
}

void HashMap::rehash() {
    int doubled = capacity * 2;

    std::vector<
        std::list<
            HashEntry
        >
    > more_buckets { doubled };

    for (const auto& bucket : buckets) {
        for (const auto& entry : bucket) {
            int hash = hash_function(entry.key, doubled);
            more_buckets[hash].emplace_back(entry);
        }
    }

    capacity = doubled; 
    buckets = std::move(more_buckets);
}

void HashMap::add(const std::string& key, BaseEntry *value) {
    int hash = hash_function(key, capacity);
    auto& bucket = buckets[hash];

    for (auto& entry : bucket) {
        if (entry.key == key) {
            entry.value = value;
            return;
        }
    }

    bucket.emplace_back(key, value);

    size++;

    if (static_cast<float> (size) / capacity >= loadFactor) {
        rehash();
    }
}

BaseEntry *HashMap::get(const std::string& key) {
    int hash = hash_function(key, capacity);
    auto& bucket = buckets[hash];

    for (auto& entry : bucket) {
        if (entry.key == key) {
            return entry.value;
        }
    }

    return nullptr;
}

bool HashMap::remove(const std::string& key) {
    int hash = hash_function(key, capacity);

    for (auto it = buckets[hash].begin(); it != buckets[hash].end(); it++) {
        if (it->key == key) {
            delete it->value;
            buckets[hash].erase(it);
            size--;
            return true;
        }
    }    

    return false;
}

std::vector<std::string> HashMap::key_set() {
    std::vector<std::string> keys;
    keys.reserve(size);

    for (const auto& bucket : buckets) {
        for (const auto& entry : bucket) {
            keys.emplace_back(entry.key);
        }
    }

    return keys;
}

int HashMap::get_capacity() {
    return capacity;
}

int HashMap::get_size() {
    return size;
}

