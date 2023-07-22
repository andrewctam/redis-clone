#include "gtest/gtest.h"

#include "hashmap.h"
#include "../src/hashmap.cpp"

TEST(HashMapTests, AddAndRemove) {
    HashMap<std::string> hashmap;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    hashmap.add("key1", &value1);
    hashmap.add("key2", &value2);

    EXPECT_EQ(*hashmap.get("key1"), value1);
    EXPECT_EQ(*hashmap.get("key2"), value2);

    EXPECT_EQ(hashmap.remove("key1"), true);

    EXPECT_EQ(hashmap.get("key1"), nullptr);

    EXPECT_EQ(hashmap.remove("key1"), false);
    EXPECT_EQ(hashmap.remove("random"), false);
}

TEST(HashMapTests, ReplaceKey) {
    HashMap<std::string> hashmap;

    std::string value1 = "Value1";
    std::string value2 = "Value2";

    hashmap.add("key1", &value1);
    hashmap.add("key2", &value2);

    EXPECT_EQ(*hashmap.get("key1"), value1);
    EXPECT_EQ(*hashmap.get("key2"), value2);

    std::string value3 = "Value3";
    std::string value4 = "Value4";

    hashmap.add("key1", &value3);
    hashmap.add("key2", &value4);

    EXPECT_EQ(*hashmap.get("key1"), value3);
    EXPECT_EQ(*hashmap.get("key2"), value4);
}

TEST(HashMapTests, Rehashing) {
    int init_size = 5;

    HashMap<std::string> hashmap (init_size);

    std::vector<std::string> keys {"1", "2", "3", "4", "5", "6", "7"};
    std::vector<std::string> vals {"A", "B", "C", "D", "E", "F", "G"};

    for (int i = 0; i < keys.size(); i++) {
        hashmap.add(keys[i], &vals[i]);
    }
    

    EXPECT_EQ(hashmap.get_capacity(), init_size * 2);
    EXPECT_EQ(hashmap.get_size(), keys.size());

    for (int i = 0; i < keys.size(); i++) {
        EXPECT_EQ(*hashmap.get(keys[i]), vals[i]);
    }
}
