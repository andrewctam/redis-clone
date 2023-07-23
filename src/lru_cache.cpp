#include "lru_cache.h"
#include "server.h"

LRUCache::LRUCache(int inital_capacity) {
    buckets.resize(inital_capacity);
}

int LRUCache::hash_function(const std::string& key, int elements) {
    std::hash<std::string> hasher;
    return hasher(key) % elements;
}

int LRUCache::get_capacity() {
    return buckets.size();
}

int LRUCache::get_size() {
    return size;
}

void LRUCache::rehash() {
    auto doubled = buckets.size() * 2;

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

    buckets = std::move(more_buckets);
}

void LRUCache::add(const std::string& key, BaseEntry *value) {
    int hash = hash_function(key, buckets.size());
    auto& bucket = buckets[hash];

    for (auto& entry : bucket) {
        if (entry.key == key) {
            entry.value = value;
            return;
        }
    }

    bucket.emplace_back(key, value);

    size++;

    if (static_cast<float> (size) / buckets.size() >= loadFactor) {
        rehash();
    }
}

BaseEntry *LRUCache::get(const std::string& key) {
    int hash = hash_function(key, buckets.size());
    auto& bucket = buckets[hash];

    for (auto it = bucket.begin(); it != bucket.end(); it++) {
        if (it->key == key) {
            seconds::rep unix_times = time_secs();
            seconds::rep expire = it->value->expiration;
            if (expire > 0 && expire <= unix_times) {
                delete it->value;
                bucket.erase(it);
                size--;
                return nullptr;
            }

            return it->value;
        }
    }

    return nullptr;
}

bool LRUCache::remove(const std::string& key) {
    int hash = hash_function(key, buckets.size());
    auto& bucket = buckets[hash];

    for (auto it = bucket.begin(); it != bucket.end(); it++) {
        if (it->key == key) {
            delete it->value;
            bucket.erase(it);
            size--;
            return true;
        }
    }    

    return false;
}

std::vector<std::string> LRUCache::key_set() {
    std::vector<std::string> keys;
    keys.reserve(size);

    for (const auto& bucket : buckets) {
        for (const auto& entry : bucket) {
            keys.emplace_back(entry.key);
        }
    }

    return keys;
}

bool LRUCache::set_expire(const std::string& key, std::time_t time) {
    int hash = hash_function(key, buckets.size());
    auto& bucket = buckets[hash];

    for (auto& entry : bucket) {
        if (entry.key == key) {
            entry.value->expiration = time;
            return true;
        }
    }

    return false;
}

