#include "gtest/gtest.h"
#include <thread>
#include <zmq.hpp>

#include "consistent-hashing.hpp"
#include "globals.hpp"

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
    socket.send(zmq::buffer("OK"), zmq::send_flags::none);
    std::string first = request.to_string();

    std::cout << first << std::endl;
    //send first reply on second
    res = socket.recv(request, zmq::recv_flags::none);
    socket.send(zmq::buffer(first), zmq::send_flags::none);
}

std::string send_str(ServerNode *node, std::string str) {
    zmq::message_t request;
    node->send(str);
    node->recv(request);
    //extra \0 without substr
    std::string res = request.to_string();

    return res.substr(0, res.size());
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

    EXPECT_EQ(ch_ring.get(hash_2), node_b); //128
    EXPECT_EQ(ch_ring.get(hash_152), node_a); //289
    EXPECT_EQ(ch_ring.get(hash_339), node_c); //342
    EXPECT_EQ(ch_ring.get(hash_343), node_b); //128
}


TEST(ConsistentHashingTests, ConnectionsUpdate) {
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
    EXPECT_EQ(send_str(node2_two, "2"), "c\nc\n");

    ServerNode *node2_four = ch_ring.get_by_pid("four");
    EXPECT_EQ(node2_four->is_leader, false);
    EXPECT_EQ(send_str(node2_four, "4"), "36\n36\n");


    two.join();
    four.join();
}
