#include "gtest/gtest.h"

#include "linked_list.h"

TEST(LinkedListTests, Empty) {
    LinkedList list;
    EXPECT_EQ(list.get_size(), 0);
    EXPECT_EQ(list.values().size(), 0);
}

TEST(LinkedListTests, Adding) {
    LinkedList list;

    list.add_end(new StringEntry("1"));
    list.add_end(new StringEntry("2"));
    list.add_front(new StringEntry("0"));
    list.add_end(new StringEntry("3"));

    EXPECT_EQ(list.get_size(), 4);

    std::vector<std::string> exp {"0", "1", "2", "3"};
    EXPECT_EQ(list.values(), exp);
}

TEST(LinkedListTests, Get) {
    LinkedList list;

    list.add_end(new StringEntry("1"));
    list.add_end(new StringEntry("2"));
    list.add_front(new StringEntry("0"));
    list.add_end(new StringEntry("3"));

    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(
            dynamic_cast<StringEntry *>(list.get(i))->value, 
            std::to_string(i));
    }    
}


TEST(LinkedListTests, Remove) {
    LinkedList list;

    for (int i = 0; i < 5; i++) {
        list.add_end(new StringEntry(std::to_string(i)));
    }

    EXPECT_EQ(list.get_size(), 5);

    std::vector<std::string> exp {"0", "1", "2", "3", "4"};
    EXPECT_EQ(list.values(), exp);

    for (int i = 0; i < 5; i++) {
        list.remove_end();
    }

    EXPECT_EQ(list.get_size(), 0);
    EXPECT_EQ(list.values().size(), 0);
}
TEST(LinkedListTests, RemoveLots) {
    LinkedList list;

    for (int i = 0; i < 10; i++) {
        list.add_end(new StringEntry(std::to_string(i)));
    }

    list.remove_end(); //9
    list.remove_end(); //8

    list.remove_front(); //0

    list.remove(2); //3

    EXPECT_EQ(list.get_size(), 6);
    std::vector<std::string> exp {"1", "2", "4", "5", "6", "7"};

    EXPECT_EQ(list.values(), exp);
}

TEST(LinkedListTests, RemoveNode) {
    LinkedList list;

    for (int i = 0; i < 5; i++) {
        list.add_end(new StringEntry(std::to_string(i)));
    }

    Node *hd = list.get_node(0);
    Node *two = list.get_node(2);
    Node *tl = list.get_node(4);

    list.remove_node(hd);
    list.remove_node(two);
    list.remove_node(tl);

    EXPECT_EQ(list.get_size(), 2);
    std::vector<std::string> exp {"1", "3"};

    EXPECT_EQ(list.values(), exp);
}

TEST(LinkedListTests, MixedTypes) {
    LinkedList list;

    list.add_end(new StringEntry("a"));
    list.add_end(new CacheEntry("2", nullptr));
    list.add_end(new IntEntry(3));

    EXPECT_EQ(dynamic_cast<StringEntry *>(list.get(0))->value, "a");
    EXPECT_EQ(dynamic_cast<CacheEntry *>(list.get(1))->key, "2");
    EXPECT_EQ(dynamic_cast<IntEntry *>(list.get(2))->value, 3);

    EXPECT_EQ(list.get_size(), 3);
    std::vector<std::string> exp {"a", "2", "3"};

    EXPECT_EQ(list.values(), exp);

}
