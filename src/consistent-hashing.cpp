#include "consistent-hashing.h"

ServerNode::ServerNode(std::string pid, std::string endpoint) :
    pid(pid), 
    context(new zmq::context_t(1)),
    socket(new zmq::socket_t(*context, zmq::socket_type::req)),
    hash(std::hash<std::string>()(pid + endpoint) % 360),
    is_leader(is_leader) {

    socket->connect(endpoint);
}

ServerNode::~ServerNode() {
    delete socket;
    delete context;
}

void ConsistentHashing::add(std::string pid, std::string endpoint, bool is_leader) {
    connected.insert(new ServerNode(pid, endpoint, is_leader));
}

void ConsistentHashing::add_all(std::string internal_string) {
    std::istringstream iss(internal_string);

    std::istream_iterator<std::string> it(iss);

    while (it != std::istream_iterator<std::string>()) {
        int sep = (*it).find(",");
        bool leader = false; 
        
        std::string endpoint = (*it).substr(sep + 1);
        std::string pid;
        
        if ((*it).at(0) == '*') {
            leader = true;
            pid = (*it).substr(1, sep);
        } else {
            pid = (*it).substr(0, sep);
        }

        ServerNode *existing = get(pid + endpoint);

        if (!existing ||
            existing->pid != pid ||
            existing->socket->get(zmq::sockopt::last_endpoint) != endpoint) {

            add(pid, endpoint, leader);
        }

        ++it;
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

        ss << node->pid << "," << node->socket->get(zmq::sockopt::last_endpoint) << " ";
    }

    return ss.str();
}