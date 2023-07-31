#ifndef CONSISTENT_HASHING_H
#define CONSISTENT_HASHING_H

#include <set>
#include <string>
#include <zmq.hpp>

class ServerNode {
public:
    std::string pid;
    zmq::context_t *context;
    zmq::socket_t *socket;
    int hash;
    bool is_leader;

    ServerNode(std::string pid, std::string endpoint, bool is_leader);
    ~ServerNode();
};

struct Compare {
    using is_transparent = void;

    bool operator()(ServerNode* const &node1, ServerNode* const &node2) const {
        return node1->hash < node2->hash;
    }

    bool operator()(ServerNode* const &node, const std::string &str) const {
        return node->hash < std::hash<std::string>()(str) % 360;
    }

    bool operator()(const std::string &str, ServerNode* const&node) const {
        return std::hash<std::string>()(str) % 360 < node->hash;
    }
};

class ConsistentHashing {
private:
    std::set<ServerNode*, Compare> connected;

public:
    ConsistentHashing() {}

    void add(std::string pid, std::string endpoint, bool is_leader);
    void add_all(std::string internal_string);
    
    ServerNode *get(const std::string &str);
    ServerNode *get_by_pid(const std::string &str);

    int size() { return connected.size(); }

    std::string to_user_string();
    std::string to_internal_string();

};


#endif