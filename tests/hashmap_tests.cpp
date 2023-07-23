#include "gtest/gtest.h"

#include "hashmap.h"
#include "unix_secs.h"

TEST(HashMapTests, AddAndRemove) {
    HashMap hashmap;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    hashmap.add("key1", new StringEntry(value1));
    hashmap.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key1"))->value, 
        value1);
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key2"))->value, 
        value2);

    EXPECT_EQ(hashmap.remove("key1"), true);

    EXPECT_EQ(hashmap.get("key1"), nullptr);

    EXPECT_EQ(hashmap.remove("key1"), false);
    EXPECT_EQ(hashmap.remove("random"), false);
}

TEST(HashMapTests, ReplaceKey) {
    HashMap hashmap;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    hashmap.add("key1", new StringEntry(value1));
    hashmap.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key1"))->value, 
        value1);
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key2"))->value, 
        value2);

    std::string value3 = "Value3";
    std::string value4 = "Value4";

    hashmap.add("key1", new StringEntry(value3));
    hashmap.add("key2", new StringEntry(value4));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key1"))->value, 
        value3);
        
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key2"))->value, 
        value4);
}

TEST(HashMapTests, Rehashing) {
    int init_size = 5;

    HashMap hashmap (init_size);

    std::vector<std::string> keys {"1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> vals {"A", "B", "C", "D", "E", "F", "G"};

    for (int i = 0; i < keys.size(); i++) {
        hashmap.add(keys[i], new StringEntry(vals[i]));
    }

    EXPECT_EQ(hashmap.get_capacity(), init_size * 2);
    EXPECT_EQ(hashmap.get_size(), keys.size());

    for (int i = 0; i < keys.size(); i++) {
        EXPECT_EQ(
            dynamic_cast<StringEntry*>(hashmap.get(keys[i]))->value, 
            vals[i]);
    }
}


TEST(HashMapTests, KeySet) {
    int init_size = 5;

    HashMap hashmap (init_size);

    std::vector<std::string> keys {"1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> vals {"A", "B", "C", "D", "E", "F", "G"};

    for (int i = 0; i < keys.size(); i++) {
        hashmap.add(keys[i], new StringEntry(vals[i]));
    }

    std::vector<std::string> strs = hashmap.key_set();
    EXPECT_EQ(keys.size(), strs.size());

    std::set<std::string> set1(keys.begin(), keys.end());
    std::set<std::string> set2(strs.begin(), strs.end());

    EXPECT_EQ(set1, set2);
}

TEST(HashMapTests, Expire) {
    HashMap hashmap;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    hashmap.add("key1", new StringEntry(value1));
    hashmap.add("key2", new StringEntry(value2));

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key1"))->expiration, 
        0);

    seconds::rep future = time_secs() + 1000;
    hashmap.set_expire("key1", future);
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(hashmap.get("key1"))->expiration, 
        future);
    
    
    hashmap.set_expire("key1", 1);
    EXPECT_EQ(hashmap.get_size(), 2); //lazy delete
    EXPECT_EQ(hashmap.get("key1"), nullptr);
    EXPECT_EQ(hashmap.get_size(), 1);

}