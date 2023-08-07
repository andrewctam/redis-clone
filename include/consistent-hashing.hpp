#ifndef CONSISTENT_HASHING_H
#define CONSISTENT_HASHING_H

#include <set>
#include <string>
#include <zmq.hpp>
#include <mutex>

#include "unix_times.hpp"

int hash_function(const std::string &str);
constexpr milliseconds::rep ACCEPTABLE_TIME = 5000;

class ServerNode {
private:
    zmq::context_t *context;
    zmq::socket_t *socket;
    milliseconds::rep last_ping;

public:
    std::string pid;
    std::string endpoint;

    int hash;
    bool is_leader;

    ServerNode(std::string pid, std::string endpoint, bool is_leader);
    ~ServerNode();

    bool send(const std::string &str);
    bool recv(zmq::message_t &request);

    void refresh_last_ping() { last_ping = time_ms(); }
    void mark_for_removal() { last_ping = 0; }
    bool too_long_since_ping() { return time_ms() > last_ping + ACCEPTABLE_TIME; }
};


struct Compare {
    using is_transparent = void;

    bool operator()(ServerNode* const &node1, ServerNode* const &node2) const {
        if (node1->hash == node2->hash) {
            return node1->pid < node2->pid;
        }
        
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
    std::string this_pid;
public:
    std::recursive_mutex mutex;

    zmq::context_t *dealer_context;
    zmq::socket_t *dealer_socket;

    // param mainly used to pass in controlled var for test cases, otherwise just use default
    ConsistentHashing(std::string pid = std::to_string(getpid()));
    ~ConsistentHashing();
    
    ServerNode *add(std::string pid, std::string endpoint, bool is_leader);

    ServerNode *get(const std::string &str);
    ServerNode *get_next_node(ServerNode *node);
    ServerNode *get_by_pid(const std::string &str);

    void set_up_dealer();
    bool dealer_send(const std::string &msg);

    std::string to_user_string();
    std::string to_internal_string();
    void update(std::string internal_string);
    void send_extracted_cache(ServerNode *left, ServerNode *right, bool wrap_around);

    int size() { return connected.size(); }
    bool is_begin(ServerNode *node) { return size() > 0 && node == *connected.begin(); }

    void clean_up_old_nodes();
};


#endif