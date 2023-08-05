#include "gtest/gtest.h"

#include "lru_cache.hpp"
#include "unix_times.hpp"
#include "linked_list.hpp"
#include "entries/list_entry.hpp"

TEST(LRUCacheTests, Clear) {
    LRUCache cache { };

    cache.add("key1", new StringEntry("Value 1"));
    cache.add("key2", new StringEntry("Value 2"));
    cache.add("key3", new StringEntry("Value 3"));
    
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
}

TEST(LRUCacheTests, AddAndRemove) {
    LRUCache cache { };

    StringEntry *v1 = new StringEntry("Value 1");
    StringEntry *v2 = new StringEntry("Value 2");

    cache.add("key1", v1);
    cache.add("key2", v2);

    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key1"))->value, 
        "Value 1");
    EXPECT_EQ(
        dynamic_cast<StringEntry*>(cache.get("key2"))->value, 
        "Value 2");

    EXPECT_EQ(cache.remove("key1"), v1);

    EXPECT_EQ(cache.get("key1"), nullptr);

    EXPECT_EQ(cache.remove("key1"), nullptr);
    EXPECT_EQ(cache.remove("random key"), nullptr);

    EXPECT_EQ(cache.remove("key2"), v2);
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
    EXPECT_EQ(cache.get("3"), nullptr);
}


TEST(LRUCacheTests, LRUTypes) {
    LRUCache cache { 2, 2 };

    ListEntry *list_entry = new ListEntry();
    LinkedList *list = list_entry->list;
    list->add_end(new StringEntry("1"));
    list->add_end(new StringEntry("2"));
    list->add_end(new StringEntry("3"));

    cache.add("list", list_entry);
    EXPECT_EQ(cache.get("list")->to_string(), "1 2 3");

    cache.add("num", new IntEntry(1));
    EXPECT_EQ(cache.get("num")->to_string(), "1");

    std::vector<std::string> exp1 {"list", "num"};
    EXPECT_EQ(cache.key_set(), exp1);

    cache.add("str", new StringEntry("b"));
    std::vector<std::string> exp2 {"num", "str"};
    EXPECT_EQ(cache.key_set(), exp2);
    EXPECT_EQ(cache.get("list"), nullptr);

    cache.add("str2", new StringEntry("c"));    
    std::vector<std::string> exp3 {"str", "str2"};
    EXPECT_EQ(cache.key_set(), exp3);
    EXPECT_EQ(cache.get("num"), nullptr);
}

TEST(LRUCacheTests, ImportConstruct) {
    std::string str = 
    "key1\n"
    "value1\n"
    "key2\n"
    "value2\n"
    "key3\n"
    "value3\n";

    LRUCache cache { str, 5, 5 };

    EXPECT_EQ(cache.size(), 3);
    EXPECT_EQ(cache.get("key1")->to_string(), "value1");
    EXPECT_EQ(cache.get("key2")->to_string(), "value2");
    EXPECT_EQ(cache.get("key3")->to_string(), "value3");
}


TEST(LRUCacheTests, Import) {
    LRUCache cache { 5, 5 };

    EXPECT_EQ(cache.import(""), true);
    EXPECT_EQ(cache.size(), 0);

    cache.add("key0", new StringEntry("value0"));
    cache.add("key1", new StringEntry("1"));

    std::string import_str = 
    "key1\n"
    "value1\n"
    "key2\n"
    "2\n"
    "key3\n"
    "str 2 3\n";

    EXPECT_EQ(cache.import(import_str), true);

    EXPECT_EQ(cache.size(), 4);
    EXPECT_EQ(cache.get("key0")->to_string(), "value0");
    EXPECT_EQ(cache.get("key0")->get_type(), EntryType::str);

    EXPECT_EQ(cache.get("key1")->to_string(), "value1");
    EXPECT_EQ(cache.get("key1")->get_type(), EntryType::str);

    EXPECT_EQ(cache.get("key2")->to_string(), "2");
    EXPECT_EQ(cache.get("key2")->get_type(), EntryType::integer);

    EXPECT_EQ(cache.get("key3")->to_string(), "str 2 3");
    EXPECT_EQ(cache.get("key3")->get_type(), EntryType::list);
}


TEST(LRUCacheTests, IncompleteImport) {
    LRUCache cache { 5, 5 };

    std::string import_str = 
    "key1\n"
    "value1\n"
    "key2\n";

    EXPECT_EQ(cache.import(import_str), false);

    EXPECT_EQ(cache.size(), 1);
    EXPECT_EQ(cache.get("key1")->to_string(), "value1");
    EXPECT_EQ(cache.get("key2"), nullptr);
}


TEST(LRUCacheTests, Extract) {
    LRUCache cache { 10, 10 };

    std::string import_str = 
    "key1\n" // 322
    "1\n"
    "key2\n" // 316
    "2\n"
    "key3\n" // 18
    "3\n"
    "key4\n" // 38
    "4\n" 
    "key5\n" // 56
    "5\n"; 

    EXPECT_EQ(cache.import(import_str), true);
    EXPECT_EQ(cache.size(), 5);

    std::vector<std::string> strs = cache.extract({ 0, 20, 40, 320 });
    EXPECT_EQ(strs.size(), 3);
    EXPECT_EQ(strs[0], "key3\n3\n");
    EXPECT_EQ(strs[1], "key4\n4\n");
    EXPECT_TRUE(strs[2] == "key2\n2\nkey5\n5\n" || strs[2] == "key5\n5\nkey2\n2\n");

    strs = cache.extract({ 20, 40, 320 });
    EXPECT_EQ(strs.size(), 2);
    EXPECT_EQ(strs[0], "key4\n4\n");
    EXPECT_TRUE(strs[1] == "key2\n2\nkey5\n5\n" || strs[1] == "key5\n5\nkey2\n2\n");

    strs = cache.extract({ 322, 20, 56 });
    EXPECT_EQ(strs.size(), 2);
    EXPECT_TRUE(strs[0] == "key1\n1\nkey3\n3\n" || strs[0] == "key3\n3\nkey1\n1\n");
    EXPECT_EQ(strs[1], "key4\n4\n");

    strs = cache.extract({ 310, 322, 20, 56 });
    EXPECT_EQ(strs.size(), 3);
    EXPECT_TRUE(strs[0] == "key2\n2\n");
    EXPECT_TRUE(strs[1] == "key1\n1\nkey3\n3\n" || strs[1] == "key3\n3\nkey1\n1\n");
    EXPECT_EQ(strs[2], "key4\n4\n");

    strs = cache.extract({ 0, 18 });
    EXPECT_EQ(strs.size(), 1);
    EXPECT_EQ(strs[0], "");

    strs = cache.extract({ 0 });
    EXPECT_EQ(strs.size(), 0);

    strs = cache.extract({ });
    EXPECT_EQ(strs.size(), 0);

    strs = cache.extract({ 0, 38 });
    EXPECT_EQ(strs.size(), 1);
    EXPECT_EQ(strs[0], "key3\n3\n");

    strs = cache.extract({ 0, 360 });
    EXPECT_EQ(strs.size(), 1);
    EXPECT_EQ(strs[0].size(), import_str.size());

}
