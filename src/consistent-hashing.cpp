#include "consistent-hashing.hpp"
#include <unordered_set>

int hash_function(const std::string &str) {
    return std::hash<std::string>()(str) % 360;
}

ServerNode::ServerNode(std::string pid, std::string endpoint, bool is_leader) :
    pid(pid), 
    endpoint(endpoint),
    hash(hash_function(pid + endpoint)),
    is_leader(is_leader) {

    if (std::to_string(getpid()) != pid) {
        context = new zmq::context_t(1);
        socket = new zmq::socket_t(*context, zmq::socket_type::req);
        socket->connect(endpoint);
    } else {
        context = nullptr;
        socket = nullptr;
    }
}

ServerNode::~ServerNode() {
    if (socket) {
        delete socket;
    }
    
    if (context) {
        delete context;
    }
}

bool ServerNode::send(const std::string &str) {
    if (!socket) {
        return false;
    }

    socket->send(zmq::buffer(str), zmq::send_flags::none);
    return true;
}

bool ServerNode::recv(zmq::message_t &request) {
    if (!socket) {
        return false;
    }
    zmq::recv_result_t res = socket->recv(request, zmq::recv_flags::none);
    return true;
}

ConsistentHashing::~ConsistentHashing() {
    if (dealer_active) {
        delete dealer_socket;
        delete dealer_context;
    }
}

void ConsistentHashing::set_up_dealer() {
    if (!dealer_active) {
        dealer_active = true;
        dealer_context = new zmq::context_t(1);
        dealer_socket = new zmq::socket_t(*dealer_context, zmq::socket_type::dealer); 
        dealer_socket->set(zmq::sockopt::rcvtimeo, 200);

        std::string my_pid = std::to_string(getpid());
        for (auto it = connected.begin(); it != connected.end(); ++it) {
            if ((*it)->pid != my_pid) {
                dealer_socket->connect((*it)->endpoint);
            }
            
        }
    }
}

ServerNode *ConsistentHashing::add(std::string pid, std::string endpoint, bool is_leader) {
    ServerNode *node = new ServerNode(pid, endpoint, is_leader); 
    connected.insert(node);

    if (dealer_active && std::to_string(getpid()) != pid) {
        dealer_socket->connect(endpoint);
    }

    return node;
}

void ConsistentHashing::update(std::string internal_string) {
    std::istringstream iss(internal_string);
    std::istream_iterator<std::string> it(iss);

    std::unordered_set<ServerNode *> nodes;
    while (it != std::istream_iterator<std::string>()) {
        int sep = (*it).find(",");
        bool leader = false; 
        
        std::string endpoint = (*it).substr(sep + 1);
        std::string pid;
        
        if ((*it).at(0) == '*') {
            leader = true;
            pid = (*it).substr(1, sep - 1);
        } else {
            pid = (*it).substr(0, sep);
        }

        ServerNode *existing = get(pid + endpoint);

        if (!existing ||
            existing->pid != pid ||
            existing->endpoint != endpoint) {

            nodes.insert(add(pid, endpoint, leader));
        } else {
            nodes.insert(existing);
        }

        ++it;
    }

    // remove nodes that were not in the latest update
    auto it_rem = connected.begin();
    while (it_rem != connected.end()) {
        if (nodes.find(*it_rem) == nodes.end()) {
            it_rem = connected.erase(it_rem);
        } else {
            it_rem++;
        }
    }
}

ServerNode *ConsistentHashing::get(const std::string &str) {
    auto it = connected.lower_bound(str);
    if (it == connected.end()) {
        it = connected.begin();
    }

    return *it;
}


ServerNode *ConsistentHashing::get_by_pid(const std::string &target) {
    if (target.size() == 0) {
        return nullptr;
    }
    
    for (auto it = connected.begin(); it != connected.end(); ++it) {
        ServerNode *node = *it;
        if (node->pid == target) {
            return node;
        }
    }
    return nullptr;
}

std::string ConsistentHashing::to_user_string() {
    std::stringstream ss;

    for (auto it = connected.begin(); it != connected.end(); ++it) {
        ServerNode *node = *it;
        if (node->is_leader) {
            ss << "Leader: " << node->pid << "\n";
        } else {
            ss << "Worker: " << node->pid << "\n";
        }
    }

    return ss.str();
}

std::string ConsistentHashing::to_internal_string() {
    std::stringstream ss;

    for (auto it = connected.begin(); it != connected.end(); ++it) {
        ServerNode *node = *it;
        if (node->is_leader) {
            ss << "*";
        }

        ss << node->pid << "," << node->endpoint << " ";
    }

    return ss.str();
}