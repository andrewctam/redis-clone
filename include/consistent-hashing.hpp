#ifndef CONSISTENT_HASHING_H
#define CONSISTENT_HASHING_H

#include <set>
#include <string>
#include <zmq.hpp>

int hash_function(const std::string &str);

class ServerNode {
private:
    zmq::context_t *context;
    zmq::socket_t *socket;
    
public:
    std::string pid;
    std::string endpoint;
    
    int hash;
    bool is_leader;

    ServerNode(std::string pid, std::string endpoint, bool is_leader);
    ~ServerNode();

    bool send(const std::string &str);
    bool recv(zmq::message_t &request);
};


struct Compare {
    using is_transparent = void;

    bool operator()(ServerNode* const &node1, ServerNode* const &node2) const {
        return node1->hash < node2->hash;
    }

    bool operator()(ServerNode* const &node, const std::string &str) const {
        return node->hash < hash_function(str);
    }

    bool operator()(const std::string &str, ServerNode* const&node) const {
        return hash_function(str) < node->hash;
    }
};

class ConsistentHashing {
private:
    std::set<ServerNode*, Compare> connected;
    bool dealer_active = false;
public:
    zmq::context_t *dealer_context;
    zmq::socket_t *dealer_socket;

    ConsistentHashing() {}
    ~ConsistentHashing();
    
    void set_up_dealer();
    
    ServerNode *add(std::string pid, std::string endpoint, bool is_leader);
    void update(std::string internal_string);
    
    ServerNode *get(const std::string &str);

    ServerNode *get_by_pid(const std::string &str);

    int size() { return connected.size(); }

    std::string to_user_string();
    std::string to_internal_string();

};


#endif