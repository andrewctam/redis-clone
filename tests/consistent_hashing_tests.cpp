#include "gtest/gtest.h"
#include <thread>
#include <zmq.hpp>

#include "consistent-hashing.hpp"


void create_socket(std::string port) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:" + port);

    //echo req
    zmq::message_t request;
    zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
    socket.send(zmq::buffer(request.to_string()), zmq::send_flags::none);
}

std::string send_str(ServerNode *node, std::string str) {
    zmq::message_t request;
    node->send(str);
    node->recv(request);
    //extra \0 without substr
    return request.to_string().substr(0, str.size());
}

TEST(ConsistentHashingTests, Connections) {
    std::thread one(create_socket, "25551");
    std::thread two(create_socket, "25552");
    std::thread three(create_socket, "25553");

    ConsistentHashing ch_ring;

    EXPECT_EQ(ch_ring.add("one", "tcp://localhost:25551", true) != nullptr, true);
    EXPECT_EQ(ch_ring.add("two", "tcp://localhost:25552", false) != nullptr, true);
    EXPECT_EQ(ch_ring.add("three", "tcp://localhost:25553", false) != nullptr, true);

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

TEST(ConsistentHashingTests, ConnectionsUpdate) {
    std::thread one(create_socket, "25551");
    std::thread two(create_socket, "25552");
    std::thread three(create_socket, "25553");

    ConsistentHashing ch_ring;
    EXPECT_EQ(ch_ring.add("one", "tcp://localhost:25551", false) != nullptr, true);

    ServerNode *node_one = ch_ring.get_by_pid("one");

    EXPECT_EQ(send_str(node_one, "Hi from 1"), "Hi from 1");
    one.join();

    ConsistentHashing ch_ring2;
    EXPECT_EQ(ch_ring2.add("two", "tcp://localhost:25552", true) != nullptr, true);
    EXPECT_EQ(ch_ring2.add("three", "tcp://localhost:25553", false) != nullptr, true);

    std::string internal_str = ch_ring2.to_internal_string();
    ch_ring.update(internal_str);
    EXPECT_EQ(ch_ring.size(), 2);
    
    EXPECT_EQ(ch_ring.get_by_pid("one"), nullptr);

    ServerNode *node_two = ch_ring.get_by_pid("two");
    EXPECT_TRUE(node_two->is_leader);
    EXPECT_EQ(send_str(node_two, "Hi from 2"), "Hi from 2");

    ServerNode *node_three = ch_ring.get_by_pid("three");
    EXPECT_FALSE(node_three->is_leader);
    EXPECT_EQ(send_str(node_three, "Hi from 3"), "Hi from 3");

    two.join();
    three.join();
}


TEST(ConsistentHashingTests, ConnectionsUpdateExisting) {
    std::thread one(create_socket, "25551");
    std::thread two(create_socket, "25552");
    std::thread three(create_socket, "25553");

    ConsistentHashing ch_ring;
    EXPECT_EQ(ch_ring.add("one", "tcp://localhost:25551", false) != nullptr, true);

    ConsistentHashing ch_ring2;
    EXPECT_EQ(ch_ring2.add("one", "tcp://localhost:25551", false) != nullptr, true);
    EXPECT_EQ(ch_ring2.add("two", "tcp://localhost:25552", true) != nullptr, true);
    EXPECT_EQ(ch_ring2.add("three", "tcp://localhost:25553", false) != nullptr, true);

    std::string internal_str = ch_ring2.to_internal_string();
    ch_ring.update(internal_str);
    EXPECT_EQ(ch_ring.size(), 3);
    
    ServerNode *node_one = ch_ring.get_by_pid("one");
    EXPECT_EQ(node_one->is_leader, false);
    EXPECT_EQ(send_str(node_one, "Hi from 1"), "Hi from 1");

    ServerNode *node_two = ch_ring.get_by_pid("two");
    EXPECT_EQ(node_two->is_leader, true);
    EXPECT_EQ(send_str(node_two, "Hi from 2"), "Hi from 2");

    ServerNode *node_three = ch_ring.get_by_pid("three");
    EXPECT_EQ(node_three->is_leader, false);
    EXPECT_EQ(send_str(node_three, "Hi from 3"), "Hi from 3");

    one.join();
    two.join();
    three.join();
}


TEST(ConsistentHashingTests, ConsistentHashing) {
    std::thread one(create_socket, "25551");
    std::thread two(create_socket, "25552");
    std::thread three(create_socket, "25553");

    ConsistentHashing ch_ring;

    EXPECT_EQ(ch_ring.add("a", "tcp://localhost:25551", true) != nullptr, true);
    EXPECT_EQ(ch_ring.add("b", "tcp://localhost:25552", false) != nullptr, true);
    EXPECT_EQ(ch_ring.add("c", "tcp://localhost:25553", false) != nullptr, true);
    
    ServerNode *node_a = ch_ring.get_by_pid("a");
    EXPECT_EQ(send_str(node_a, "Hi"), "Hi");
    ServerNode *node_b = ch_ring.get_by_pid("b");
    EXPECT_EQ(send_str(node_b, "Hi"), "Hi");
    ServerNode *node_c = ch_ring.get_by_pid("c");
    EXPECT_EQ(send_str(node_c, "Hi"), "Hi");

    one.join();
    two.join();
    three.join();

    // std::cout << "node_a (" << reinterpret_cast<void *>(node_a) <<  ") hash: " << node_a->hash << std::endl; //289
    // std::cout << "node_b (" << reinterpret_cast<void *>(node_b) <<  ") hash: " << node_b->hash << std::endl; //128
    // std::cout << "node_c (" << reinterpret_cast<void *>(node_c) <<  ") hash: " << node_c->hash << std::endl; //342

    std::string hash_2 = "two";
    std::string hash_152 = "cool";
    std::string hash_339 = "a";
    std::string hash_343 = "do";

    EXPECT_EQ(ch_ring.get(hash_2), node_b); //128
    EXPECT_EQ(ch_ring.get(hash_152), node_a); //289
    EXPECT_EQ(ch_ring.get(hash_339), node_c); //342
    EXPECT_EQ(ch_ring.get(hash_343), node_b); //128
}
