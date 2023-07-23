#include "gtest/gtest.h"

#include "lru_cache.h"
#include "unix_times.h"

TEST(LRUCacheTests, AddAndRemove) {
    LRUCache cache;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    cache.add("key1", new StringEntry(value1));
    cache.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->value, 
        value1);
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key2"))->value, 
        value2);

    EXPECT_EQ(cache.remove("key1"), true);

    EXPECT_EQ(cache.get("key1"), nullptr);

    EXPECT_EQ(cache.remove("key1"), false);
    EXPECT_EQ(cache.remove("random"), false);
}

TEST(LRUCacheTests, ReplaceKey) {
    LRUCache cache;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    cache.add("key1", new StringEntry(value1));
    cache.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->value, 
        value1);
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key2"))->value, 
        value2);

    std::string value3 = "Value3";
    std::string value4 = "Value4";

    cache.add("key1", new StringEntry(value3));
    cache.add("key2", new StringEntry(value4));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->value, 
        value3);
        
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key2"))->value, 
        value4);
}

TEST(LRUCacheTests, Rehashing) {
    int init_size = 5;

    LRUCache cache (init_size);

    std::vector<std::string> keys {"1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> vals {"A", "B", "C", "D", "E", "F", "G"};

    for (int i = 0; i < keys.size(); i++) {
        cache.add(keys[i], new StringEntry(vals[i]));
    }

    EXPECT_EQ(cache.get_capacity(), init_size * 2);
    EXPECT_EQ(cache.get_size(), keys.size());

    for (int i = 0; i < keys.size(); i++) {
        EXPECT_EQ(
            dynamic_cast<StringEntry*>(cache.get(keys[i]))->value, 
            vals[i]);
    }
}


TEST(LRUCacheTests, KeySet) {
    int init_size = 5;

    LRUCache cache (init_size);

    std::vector<std::string> keys {"1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> vals {"A", "B", "C", "D", "E", "F", "G"};

    for (int i = 0; i < keys.size(); i++) {
        cache.add(keys[i], new StringEntry(vals[i]));
    }

    std::vector<std::string> strs = cache.key_set();
    EXPECT_EQ(keys.size(), strs.size());

    std::set<std::string> set1(keys.begin(), keys.end());
    std::set<std::string> set2(strs.begin(), strs.end());

    EXPECT_EQ(set1, set2);
}

TEST(LRUCacheTests, Expire) {
    LRUCache cache;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    cache.add("key1", new StringEntry(value1));
    cache.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->expiration, 
        0);

    seconds::rep future = time_secs() + 1000;
    cache.set_expire("key1", future);
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->expiration, 
        future);
    
    
    cache.set_expire("key1", 1);
    EXPECT_EQ(cache.get_size(), 2); //lazy delete
    EXPECT_EQ(cache.get("key1"), nullptr);
    EXPECT_EQ(cache.get_size(), 1);

}