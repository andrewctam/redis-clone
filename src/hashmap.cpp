#include "hashmap.h"

template<typename V>
HashMap<V>::HashMap(): capacity(DEFAULT_INITIAL_CAPACITY) {
    capacity = DEFAULT_INITIAL_CAPACITY;
    buckets.resize(capacity);
}

template<typename V>
HashMap<V>::HashMap(int inital_capacity): capacity(inital_capacity) {
    buckets.resize(inital_capacity);
}

template<typename V>
int HashMap<V>::hash_function(const std::string& key, int elements) {
    std::hash<std::string> hasher;
    return hasher(key) % elements;
}

template<typename V>
void HashMap<V>::rehash() {
    int doubled = capacity * 2;

    std::vector<
        std::list<
            HashEntry<V>
        >
    > more_buckets(doubled);

    for (const auto& bucket : buckets) {
        for (const auto& entry : bucket) {
            int hash = hash_function(entry.key, doubled);
            more_buckets[hash].emplace_back(entry.key, entry.value);
        }
    }

    capacity = doubled;
    buckets = std::move(more_buckets);
}

template <typename V>
void HashMap<V>::add(const std::string& key, V* value) {
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

template <typename V>
V* HashMap<V>::get(const std::string& key) {
    int hash = hash_function(key, capacity);
    auto& bucket = buckets[hash];

    for (auto& entry : bucket) {
        if (entry.key == key) {
            return entry.value;
        }
    }

    return nullptr;
}

template <typename V>
bool HashMap<V>::remove(const std::string& key) {
    int hash = hash_function(key, capacity);

    for (auto it = buckets[hash].begin(); it != buckets[hash].end(); it++) {
        if (it->key == key) {
            buckets[hash].erase(it);
            size--;
            return true;
        }
    }    

    return false;
}

template <typename V>
int HashMap<V>::get_capacity() {
    return capacity;
}

template <typename V>
int HashMap<V>::get_size() {
    return size;
}


