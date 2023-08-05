
#include <unordered_set>

#include "consistent-hashing.hpp"
#include "globals.hpp"

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
    ServerNode *node = new ServerNode(pid, endpoint, is_leader); 
    connected.insert(node);

    if (dealer_active && this_pid != pid) {
        dealer_socket->connect(endpoint);
    }

    return node;
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
        std::string type = node->is_leader ? "Leader" : "Worker";

        ss << type << " pid: " << node->pid << ". Hash: " << node->hash << "\n";
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



void ConsistentHashing::update(std::string internal_string) {
    bool first_run = connected.size() == 0;

    std::istringstream iss(internal_string);
    std::istream_iterator<std::string> it(iss);

    std::unordered_set<ServerNode *> existing_nodes;
    std::unordered_set<ServerNode *> new_nodes;

    // find the immediate next node, or wrap around if last one
    ServerNode *this_node = get_by_pid(this_pid);
    ServerNode *immediate_upper = nullptr;
    if (this_node) {
        auto next_it = connected.find(this_node);
        if (next_it != connected.end()) {
            immediate_upper = (++next_it == connected.end()) ? *connected.begin() : *next_it;
        }
    }
    

    bool wrap_around = immediate_upper == *connected.begin();

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

        if (!existing ||
            existing->pid != pid ||
            existing->endpoint != endpoint) {

            new_nodes.insert(add(pid, endpoint, leader));
        } else {
            existing_nodes.insert(existing);
        }

        ++it;
    }

    if (first_run) {
        return;
    }

    // extract cache in this node's range intersecting with newly added ones
    if (new_nodes.size() > 0 && this_node && immediate_upper) {
        std::vector<int> upper_bounds;
        std::vector<ServerNode*> nodes_to_update;

        auto it3 = connected.find(this_node);
        if (it3 == connected.end()) {
            std::cerr << "???" << std::endl;
            return;
        }

        //start at node after this one
        it3++; 

        if (wrap_around) {
            // all new nodes after this to 360
            while (it3 != connected.end()) {
                nodes_to_update.emplace_back(*it3);
                upper_bounds.emplace_back((*it3)->hash);
                it3++;
            }

            it3 = connected.begin();
            // all new nodes between 0 and old immediate upper
            while (it3 != connected.end() && *it3 != immediate_upper) {
                nodes_to_update.emplace_back(*it3);
                upper_bounds.emplace_back((*it3)->hash);
                it3++;
            }
        } else {
            //take new nodes between this and old immediate upper
            while (it3 != connected.end() && *it3 != immediate_upper) {
                nodes_to_update.emplace_back(*it3);
                upper_bounds.emplace_back((*it3)->hash);
                it3++;
            }
        }

        if (upper_bounds.size() > 0) {
            upper_bounds.emplace_back(immediate_upper->hash);

            int lower = this_node->hash;
            if (wrap_around) {
                ServerNode *last = *(--connected.end());
                lower = -(last->hash);
            }

            //extract from LRU cache
            std::vector<std::string> import_strs = cache.extract(upper_bounds);
            if (import_strs.size() != nodes_to_update.size()) {
                std::cerr << "Error extracting cache" << std::endl;
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
                    socket.send(zmq::message_t(), zmq::send_flags::sndmore);
                    socket.send(zmq::buffer(import_strs[i]), zmq::send_flags::none);
                }
            }
            
        }
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
}
