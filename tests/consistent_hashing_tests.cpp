#include "gtest/gtest.h"
#include <thread>
#include <zmq.hpp>

#include "consistent-hashing.hpp"
#include "globals.hpp"
#include "worker.hpp"

std::string PREFIX = std::string { CACHE_UPDATE };

// echos one request
void create_echo_socket(std::string port) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:" + port);

    zmq::message_t request;
    zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
    socket.send(zmq::buffer(request.to_string()), zmq::send_flags::none);
}


// on the second request, sends the first reply
void create_persist_socket(std::string port) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:" + port);

    //persist first reply
    zmq::message_t request;
    zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
    socket.send(zmq::buffer("PERSISTING"), zmq::send_flags::none);
    std::string first = request.to_string();
    //send first reply on second
    res = socket.recv(request, zmq::recv_flags::none);
    socket.send(zmq::buffer(first), zmq::send_flags::none);
}

std::string send_str(ServerNode *node, std::string str, char type = 0) {
    zmq::message_t request;
    node->send(str);
    node->recv(request);
    //extra \0 without substr
    std::string res = request.to_string();
    return res.substr(0, res.size());
}

TEST(ConsistentHashingTests, PidGreater) {
    EXPECT_EQ(pid_greater("456", "123"), true);
    EXPECT_EQ(pid_greater("123", "456"), false);

    EXPECT_EQ(pid_greater("111", "99"), true);
    EXPECT_EQ(pid_greater("99", "111"), false);

    EXPECT_EQ(pid_greater("111", "101"), true);
    EXPECT_EQ(pid_greater("101", "111"), false);

    EXPECT_EQ(pid_greater("10", "0"), true);
    EXPECT_EQ(pid_greater("0", "10"), false);

    EXPECT_EQ(pid_greater("1", ""), true);
    EXPECT_EQ(pid_greater("", "1"), false);

    EXPECT_EQ(pid_greater("123", "123"), false);
}

TEST(ConsistentHashingTests, Connections) {
    std::thread one(create_echo_socket, "25551");
    std::thread two(create_echo_socket, "25552");
    std::thread three(create_echo_socket, "25553");

    ConsistentHashing ch_ring;

    EXPECT_TRUE(ch_ring.add("one", "tcp://localhost:25551", true) != nullptr);
    EXPECT_TRUE(ch_ring.add("two", "tcp://localhost:25552", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("three", "tcp://localhost:25553", false) != nullptr);

    EXPECT_EQ(ch_ring.size(), 3);
    
    ServerNode *node_one = ch_ring.get_by_pid("one");
    EXPECT_TRUE(node_one->is_leader);
    EXPECT_EQ(send_str(node_one, "Hi from 1"), "Hi from 1");
    
    ServerNode *node_two = ch_ring.get_by_pid("two");
    EXPECT_FALSE(node_two->is_leader);
    EXPECT_EQ(send_str(node_two, "Hi from 2"), "Hi from 2");
    
    ServerNode *node_three = ch_ring.get_by_pid("three");
    EXPECT_FALSE(node_three->is_leader);
    EXPECT_EQ(send_str(node_three, "Hi from 3"), "Hi from 3");

    one.join();
    two.join();
    three.join();
}

TEST(ConsistentHashingTests, ConsistentHashing) {
    std::thread one(create_echo_socket, "25551");
    std::thread two(create_echo_socket, "25552");
    std::thread three(create_echo_socket, "25553");

    ConsistentHashing ch_ring;

    EXPECT_TRUE(ch_ring.add("a", "tcp://localhost:25551", true) != nullptr);
    EXPECT_TRUE(ch_ring.add("b", "tcp://localhost:25552", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("c", "tcp://localhost:25553", false) != nullptr);
    
    ServerNode *node_a = ch_ring.get_by_pid("a");
    EXPECT_EQ(send_str(node_a, "Hi"), "Hi");
    ServerNode *node_b = ch_ring.get_by_pid("b");
    EXPECT_EQ(send_str(node_b, "Hi"), "Hi");
    ServerNode *node_c = ch_ring.get_by_pid("c");
    EXPECT_EQ(send_str(node_c, "Hi"), "Hi");

    one.join();
    two.join();
    three.join();

    std::string hash_2 = "two";
    std::string hash_152 = "cool";
    std::string hash_339 = "a";
    std::string hash_343 = "do";

    // b -> 128
    // a - > 289
    // c -> 342
    EXPECT_EQ(ch_ring.get(hash_2), node_c);
    EXPECT_EQ(ch_ring.get(hash_152), node_b);
    EXPECT_EQ(ch_ring.get(hash_339), node_a);
    EXPECT_EQ(ch_ring.get(hash_343), node_c);
}

TEST(ConsistentHashingTests, Removing) {
    ConsistentHashing ch_ring;

    // nodes creates with last ping now
    EXPECT_TRUE(ch_ring.add("one", "tcp://localhost:25551", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("two", "tcp://localhost:25552", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("three", "tcp://localhost:25553", true) != nullptr); // can not remove leader

    ServerNode *node_one = ch_ring.get_by_pid("one");
    ServerNode *node_three = ch_ring.get_by_pid("three");

    // advance time and refresh node one
    ms_offset = ACCEPTABLE_TIME + 1;
    node_one->refresh_last_ping();

    ch_ring.clean_up_old_nodes();

    EXPECT_EQ(ch_ring.size(), 2);
    EXPECT_EQ(ch_ring.get_by_pid("one"), node_one);
    EXPECT_EQ(ch_ring.get_by_pid("two"), nullptr);
    EXPECT_EQ(ch_ring.get_by_pid("three"), node_three);
    ms_offset = 0;
}

TEST(ConsistentHashingTests, MarkForRemoval) {
    ConsistentHashing ch_ring;

    EXPECT_TRUE(ch_ring.add("one", "tcp://localhost:25551", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("two", "tcp://localhost:25552", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("three", "tcp://localhost:25553", false) != nullptr);

    ServerNode *node_one = ch_ring.get_by_pid("one");
    ServerNode *node_two = ch_ring.get_by_pid("two");
    ServerNode *node_three = ch_ring.get_by_pid("three");

    node_one->mark_for_removal();
    ch_ring.clean_up_old_nodes();

    EXPECT_EQ(ch_ring.size(), 2);
    EXPECT_EQ(ch_ring.get_by_pid("one"), nullptr);
    EXPECT_EQ(ch_ring.get_by_pid("two"), node_two);
    EXPECT_EQ(ch_ring.get_by_pid("three"), node_three);
}

TEST(ConsistentHashingTests, ConnectionsUpdate) {
    cache = LRUCache();
    EXPECT_EQ(cache.size(), 0);

    std::thread two(create_echo_socket, "25552");

    // will recieve an updated string
    std::thread one(create_persist_socket, "25551");
    std::thread three(create_persist_socket, "25553");
    std::thread four(create_persist_socket, "25554");

    ConsistentHashing ch_ring {"two"}; //treat two as this node
    EXPECT_TRUE(ch_ring.add("two", "tcp://localhost:25552", true) != nullptr);

    /*        
        Worker pid: four. Hash: 173
        Worker pid: two. Hash: 183 **
        Worker pid: three. Hash: 318 
        Worker pid: one. Hash: 349
    */

    // one
    cache.add("ztybwklsxwb", new StringEntry("z")); //349
    cache.add("j", new StringEntry("j")); //59

    // four
    cache.add("vbecgeeh", new StringEntry("v")); //176
    cache.add("goxwizagf", new StringEntry("g")); //181

    // two
    cache.add("n", new StringEntry("n")); //251
    cache.add("c", new StringEntry("c")); //269

    // three
    cache.add("we", new StringEntry("we")); //337
    cache.add("weo", new StringEntry("weo")); //341


    ServerNode *node_two = ch_ring.get_by_pid("two");
    EXPECT_EQ(send_str(node_two, "Hi from 2"), "Hi from 2");

    two.join();

    ConsistentHashing ch_ring2 {"two"};
    EXPECT_TRUE(ch_ring2.add("one", "tcp://localhost:25551", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("two", "tcp://localhost:25552", true) != nullptr);
    EXPECT_TRUE(ch_ring2.add("three", "tcp://localhost:25553", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("four", "tcp://localhost:25554", false) != nullptr);


    ch_ring.update(ch_ring2.to_internal_string());
    EXPECT_EQ(ch_ring.size(), 4);
    
    ServerNode *node2_two = ch_ring.get_by_pid("two");
    EXPECT_EQ(node2_two->is_leader, true);
    EXPECT_FALSE(ch_ring.is_begin(node2_two));


    
    ServerNode *node2_one = ch_ring.get_by_pid("one");
    EXPECT_EQ(node2_one->is_leader, false);
    std::string one_cache = send_str(node2_one, "1");
    std::string exp1_v1 = PREFIX + "ztybwklsxwb\nz\nj\nj\n";
    std::string exp1_v2 = PREFIX + "j\nj\nztybwklsxwb\nz\n";
    EXPECT_TRUE(one_cache == exp1_v1 || one_cache == exp1_v2); 
    EXPECT_FALSE(ch_ring.is_begin(node2_one));


    ServerNode *node2_three = ch_ring.get_by_pid("three");
    EXPECT_EQ(node2_three->is_leader, false);
    std::string three_cache = send_str(node2_three, "2");
    std::string exp3_v1 = PREFIX + "we\nwe\nweo\nweo\n";
    std::string exp3_v2 = PREFIX + "weo\nweo\nwe\nwe\n";
    EXPECT_TRUE(three_cache == exp3_v1 || three_cache == exp3_v2); 
    EXPECT_FALSE(ch_ring.is_begin(node2_three));


    ServerNode *node2_four = ch_ring.get_by_pid("four");
    EXPECT_EQ(node2_four->is_leader, false);
    std::string four_cache = send_str(node2_four, "2");
    std::string exp4_v1 = PREFIX + "vbecgeeh\nv\ngoxwizagf\ng\n";
    std::string exp4_v2 = PREFIX + "goxwizagf\ng\nvbecgeeh\nv\n";
    EXPECT_TRUE(four_cache == exp4_v1 || four_cache == exp4_v2); 
    EXPECT_TRUE(ch_ring.is_begin(node2_four));


    one.join();
    three.join();
    four.join();
}

TEST(ConsistentHashingTests, ConnectionsUpdate2) {
    cache = LRUCache();
    EXPECT_EQ(cache.size(), 0);

    std::thread one(create_echo_socket, "25551");
    std::thread three(create_echo_socket, "25553");

    // will recieve an updated string
    std::thread two(create_persist_socket, "25552");
    std::thread four(create_persist_socket, "25554");

    ConsistentHashing ch_ring {"five"}; //treat five as this node
    EXPECT_TRUE(ch_ring.add("one", "tcp://localhost:25551", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("three", "tcp://localhost:25553", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("five", "tcp://localhost:25555", true) != nullptr);

    /*
        Leader pid: five. Hash: 112 **
        Worker pid: four. Hash: 173
        Worker pid: two. Hash: 183
        Worker pid: three. Hash: 318 **
        Worker pid: one. Hash: 349
    */
    cache.add("l", new StringEntry("l")); //134
    cache.add("36", new StringEntry("36")); //175
    cache.add("c", new StringEntry("c")); //269


    ServerNode *node_one = ch_ring.get_by_pid("one");
    EXPECT_EQ(send_str(node_one, "Hi from 1"), "Hi from 1");

    ServerNode *node_three = ch_ring.get_by_pid("three");
    EXPECT_EQ(send_str(node_three, "Hi from 3"), "Hi from 3");

    one.join();
    three.join();

    ConsistentHashing ch_ring2 {"five"};
    EXPECT_TRUE(ch_ring2.add("two", "tcp://localhost:25552", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("three", "tcp://localhost:25553", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("four", "tcp://localhost:25554", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("five", "tcp://localhost:25555", true) != nullptr);


    ch_ring.update(ch_ring2.to_internal_string());
    EXPECT_EQ(ch_ring.size(), 4);
    
    EXPECT_EQ(ch_ring.get_by_pid("one"), nullptr);

    ServerNode *node2_three = ch_ring.get_by_pid("three");
    EXPECT_EQ(node2_three->is_leader, false);

    ServerNode *node2_five = ch_ring.get_by_pid("five");
    EXPECT_EQ(node2_five->is_leader, true);
    
    ServerNode *node2_two = ch_ring.get_by_pid("two");
    EXPECT_EQ(node2_two->is_leader, false);
    std::string exp2 = PREFIX + "c\nc\n";
    EXPECT_EQ(send_str(node2_two, "2"), exp2);

    ServerNode *node2_four = ch_ring.get_by_pid("four");
    EXPECT_EQ(node2_four->is_leader, false);
    std::string exp4 = PREFIX + "36\n36\n";
    EXPECT_EQ(send_str(node2_four, "4"), exp4);


    two.join();
    four.join();
}

TEST(ConsistentHashingTests, ConnectionsUpdateWrapAround) {
    cache = LRUCache();
    EXPECT_EQ(cache.size(), 0);

    std::thread one(create_echo_socket, "25551");
    std::thread two(create_echo_socket, "25552");

    // should not receive an update string
    std::thread three(create_echo_socket, "25553");

    // will recieve an updated string
    std::thread four(create_persist_socket, "25554");
    std::thread five(create_persist_socket, "25555");

    ConsistentHashing ch_ring {"one"}; 
    EXPECT_TRUE(ch_ring.add("one", "tcp://localhost:25551", false) != nullptr);
    EXPECT_TRUE(ch_ring.add("two", "tcp://localhost:25552", false) != nullptr);

    /*
        Worker pid: five. Hash: 112 
        Worker pid: four. Hash: 173
        Worker pid: two. Hash: 183 **
        Leader pid: three. Hash: 318 
        Worker pid: one. Hash: 349 **
    */
    cache.add("l", new StringEntry("l")); //134
    cache.add("36", new StringEntry("36")); //175
    cache.add("b", new StringEntry("b")); //37


    ServerNode *node_one = ch_ring.get_by_pid("one");
    EXPECT_EQ(send_str(node_one, "Hi from 1"), "Hi from 1");

    ServerNode *node_two = ch_ring.get_by_pid("two");
    EXPECT_EQ(send_str(node_two, "Hi from 2"), "Hi from 2");

    one.join();
    two.join();

    ConsistentHashing ch_ring2 {"three"};
    EXPECT_TRUE(ch_ring2.add("one", "tcp://localhost:25551", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("two", "tcp://localhost:25552", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("three", "tcp://localhost:25553", true) != nullptr);
    EXPECT_TRUE(ch_ring2.add("four", "tcp://localhost:25554", false) != nullptr);
    EXPECT_TRUE(ch_ring2.add("five", "tcp://localhost:25555", false) != nullptr);


    ch_ring.update(ch_ring2.to_internal_string());
    EXPECT_EQ(ch_ring.size(), 5);
    

    ServerNode *node2_one = ch_ring.get_by_pid("one");
    EXPECT_EQ(node2_one->is_leader, false);

    ServerNode *node2_two = ch_ring.get_by_pid("two");
    EXPECT_EQ(node2_two->is_leader, false);

    ServerNode *node2_three = ch_ring.get_by_pid("three");
    EXPECT_EQ(node2_three->is_leader, true);
    EXPECT_EQ(send_str(node2_three, "Hi from 3"), "Hi from 3");

    ServerNode *node2_four = ch_ring.get_by_pid("four");
    EXPECT_EQ(node2_four->is_leader, false);
    std::string exp2 = PREFIX + "36\n36\n";
    EXPECT_EQ(send_str(node2_four, "4"), exp2);

    ServerNode *node2_five = ch_ring.get_by_pid("five");
    EXPECT_EQ(node2_five->is_leader, false);
    std::string exp5 = PREFIX + "l\nl\n";
    EXPECT_EQ(send_str(node2_five, "5"), exp5);

    three.join();
    four.join();
    five.join();
}

