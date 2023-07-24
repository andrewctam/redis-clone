#include "gtest/gtest.h"

#include "lru_cache.h"
#include "unix_times.h"


TEST(LRUCacheTests, AddAndRemove) {
    LRUCache cache { };

    cache.add("key1", new StringEntry("Value 1"));
    cache.add("key2", new StringEntry("Value 2"));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->value, 
        "Value 1");
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key2"))->value, 
        "Value 2");

    EXPECT_EQ(cache.remove("key1"), true);

    EXPECT_EQ(cache.get("key1"), nullptr);

    EXPECT_EQ(cache.remove("key1"), false);
    EXPECT_EQ(cache.remove("random key"), false);
}



TEST(LRUCacheTests, ReplaceKey) {
    LRUCache cache { };

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


TEST(LRUCacheTests, KeySet) {
    LRUCache cache { };

    std::vector<std::string> keys {"1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> vals {"A", "B", "C", "D", "E", "F", "G"};

    for (int i = 0; i < keys.size(); i++) {
        cache.add(keys[i], new StringEntry(vals[i]));

        if (i == 4) {
            cache.set_expire(keys[i], 1);
        }
    }

    std::vector<std::string> exp {"1", "2", "3", "4", "6", "7"};

    std::vector<std::string> key_set = cache.key_set();
    EXPECT_EQ(key_set.size(), exp.size());
    EXPECT_EQ(key_set, exp);
}


TEST(LRUCacheTests, Expire) {
    LRUCache cache { };

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    cache.add("key1", new StringEntry(value1));
    cache.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        cache.get_cache_entry("key1")->expiration, 
        0);

    seconds::rep future = time_secs() + 1000;
    cache.set_expire("key1", future);
    EXPECT_EQ(
        cache.get_cache_entry("key1")->expiration, 
        future);
    
    
    cache.set_expire("key1", 1);
    EXPECT_EQ(cache.size(), 2); //lazy delete
    EXPECT_EQ(cache.get("key1"), nullptr);
    EXPECT_EQ(cache.size(), 1);
}


TEST(LRUCacheTests, LRU) {
    LRUCache cache { 5, 5 };

    cache.add("1", new StringEntry("a"));
    cache.add("2", new StringEntry("b"));
    cache.add("3", new StringEntry("c"));
    cache.add("4", new StringEntry("d"));
    cache.add("5", new StringEntry("e"));
    std::vector<std::string> exp1 {"1", "2", "3", "4", "5"};
    EXPECT_EQ(cache.key_set(), exp1);
    
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("1"))->value, 
        "a");
    std::vector<std::string> exp2 {"2", "3", "4", "5", "1"};
    EXPECT_EQ(cache.key_set(), exp2);

    
    cache.add("2", new StringEntry("b"));
    std::vector<std::string> exp3 {"3", "4", "5", "1", "2"};
    EXPECT_EQ(cache.key_set(), exp3);


    cache.add("6", new StringEntry("b"));
    std::vector<std::string> exp4 {"4", "5", "1", "2", "6"};
    EXPECT_EQ(cache.key_set(), exp4);
}
