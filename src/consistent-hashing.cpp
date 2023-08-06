
#include <unordered_set>

#include "consistent-hashing.hpp"
#include "globals.hpp"
#include "worker.hpp"

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
    try {
        zmq::recv_result_t res = socket->recv(request, zmq::recv_flags::none);
        return true;
    } catch (...) {
        return false;
    }
}

ConsistentHashing::ConsistentHashing(std::string pid) : this_pid(pid) { }

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

        std::string my_pid = this_pid;
        for (auto it = connected.begin(); it != connected.end(); ++it) {
            if ((*it)->pid != my_pid) {
                dealer_socket->connect((*it)->endpoint);
            }
            
        }
    }
}

ServerNode *ConsistentHashing::add(std::string pid, std::string endpoint, bool is_leader) {
    mutex.lock();
    ServerNode *node = new ServerNode(pid, endpoint, is_leader); 
    connected.insert(node);

    if (dealer_active && this_pid != pid) {
        dealer_socket->connect(endpoint);
    }

    mutex.unlock();
    return node;
}

ServerNode *ConsistentHashing::get(const std::string &str) {
    if (connected.size() == 0) {
        return nullptr;
    }

    mutex.lock();
    ServerNode *node = nullptr;
    
    auto it = connected.lower_bound(str);
    if (it == connected.end()) {
        node = *(--connected.end());
    } else if ((*it)->hash == hash_function(str)) {
        node = *it;
    } else if (it == connected.begin()) { // is before but not equal to begin
        node = *(--connected.end());
    } else {
        node = *(--it);
    }

    mutex.unlock();
    return node;
}


ServerNode *ConsistentHashing::get_by_pid(const std::string &target) {
    if (target.size() == 0) {
        return nullptr;
    }
    mutex.lock();
    for (auto it = connected.begin(); it != connected.end(); ++it) {
        ServerNode *node = *it;
        if (node->pid == target) {
            mutex.unlock();
            return node;
        }
    }

    mutex.unlock();
    return nullptr;
}

bool ConsistentHashing::dealer_send(const std::string &msg) {
    if (dealer_active && dealer_socket) {
        mutex.lock();
        for (int i = 0; i < ring.size() - 1; i++) {
            dealer_socket->send(zmq::message_t(), zmq::send_flags::sndmore);
            dealer_socket->send(zmq::buffer(msg), zmq::send_flags::none);
        }
        mutex.unlock();
        return true;
    }

    return false;
}

std::string ConsistentHashing::to_user_string() {
    std::stringstream ss;

    mutex.lock();
    for (auto it = connected.begin(); it != connected.end(); ++it) {
        ServerNode *node = *it;
        std::string type = node->is_leader ? "Leader" : "Worker";

        ss << type << " pid: " << node->pid << ". Hash: " << node->hash << "\n";
    }

    mutex.unlock();
    return ss.str();
}

std::string ConsistentHashing::to_internal_string() {
    std::stringstream ss;
    mutex.lock();

    for (auto it = connected.begin(); it != connected.end(); ++it) {
        ServerNode *node = *it;
        if (node->is_leader) {
            ss << "*";
        }

        ss << node->pid << "," << node->endpoint << " ";
    }

    mutex.unlock();
    return ss.str();
}


ServerNode *ConsistentHashing::get_next_node(ServerNode *node) {
    auto next_it = connected.find(node);
    if (next_it == connected.end()) {
        return nullptr;
    }

    if (++next_it == connected.end()) { 
        return *connected.begin();
    }

    return  *next_it;
}

void ConsistentHashing::update(std::string internal_string) {
    mutex.lock();

    bool first_run = connected.size() == 0;

    std::istringstream iss(internal_string);
    std::istream_iterator<std::string> it(iss);

    std::unordered_set<ServerNode *> existing_nodes;
    std::unordered_set<ServerNode *> new_nodes;

    // find the immediate next node, or wrap around if last one
    ServerNode *this_node = get_by_pid(this_pid);
    ServerNode *next_node = get_next_node(this_node);
    bool wrap_around = is_begin(next_node);

    
    // parse internal string
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

        if (existing &&
            existing->pid == pid &&
            existing->endpoint == endpoint) {
            existing_nodes.insert(existing);
        } else {
            new_nodes.insert(add(pid, endpoint, leader));
        }

        ++it;
    }

    if (first_run) {
        mutex.unlock();
        return;
    }

    // extract cache in this node's range intersecting with newly added ones
    if (new_nodes.size() > 0 && this_node && next_node) {
        send_extracted_cache(this_node, next_node, wrap_around);
    }

    // remove nodes that were not in the latest update
    auto it2 = connected.begin();
    while (it2 != connected.end()) {
        ServerNode *cur = *it2;
        if (new_nodes.find(cur) == new_nodes.end() &&
            existing_nodes.find(cur) == existing_nodes.end()) {
            it2 = connected.erase(it2);
        } else {
            it2++;
        }
    }

    mutex.unlock();
}

// extract and send cache to nodes between left and right (exclusive of bounds)
void ConsistentHashing::send_extracted_cache(ServerNode *left, ServerNode *right, bool wrap_around) {
    if (!left || !right || connected.size() < 2) {
        return;
    }
    mutex.lock();

    std::vector<int> upper_bounds;
    std::vector<ServerNode*> nodes_to_update;

    auto it = connected.find(left);
    if (it == connected.end()) {
        std::cout << "???" << std::endl;
        mutex.unlock();
        return;
    }

    //start at node after this one
    it++; 

    // wrap around
    if (wrap_around) {
        // all new nodes after this to 360
        while (it != connected.end()) {
            nodes_to_update.emplace_back(*it);
            upper_bounds.emplace_back((*it)->hash);
            it++;
        }

        it = connected.begin();
    }
    //take new nodes between this (or begin for wrap around) and right
    while (it != connected.end() && *it != right) {
        nodes_to_update.emplace_back(*it);
        upper_bounds.emplace_back((*it)->hash);
        it++;
    }

    if (upper_bounds.size() > 0) {
        upper_bounds.emplace_back(right->hash);

        //extract from LRU cache
        std::vector<std::string> import_strs = cache.extract(upper_bounds);
        if (import_strs.size() != nodes_to_update.size()) {
            std::cout << "Error extracting cache" << std::endl;
            mutex.unlock();
            return;
        }

        // dealer socket to send cache partitions to appropriate nodes
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::dealer); 
        socket.set(zmq::sockopt::rcvtimeo, 200);

        for (int i = 0; i < nodes_to_update.size(); i++) {
            if (import_strs[i].size() > 0) {
                socket.connect(nodes_to_update[i]->endpoint);
            }
        }
        for (int i = 0; i < nodes_to_update.size(); i++) {
            if (import_strs[i].size() > 0) {
                std::string updated = CACHE_UPDATE + import_strs[i];
                socket.send(zmq::message_t(), zmq::send_flags::sndmore);
                socket.send(zmq::buffer(updated), zmq::send_flags::none);
            }
        }
    }

    mutex.unlock();
}