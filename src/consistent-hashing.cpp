#include "consistent-hashing.h"

ServerNode::ServerNode(std::string pid, std::string endpoint) :
    pid(pid), 
    context(new zmq::context_t(1)),
    socket(new zmq::socket_t(*context, zmq::socket_type::req)),
    hash(std::hash<std::string>()(pid + endpoint) % 360) {

    socket->connect(endpoint);
}


ServerNode::~ServerNode() {
    delete socket;
    delete context;
}




void ConsistentHashing::add(std::string pid, std::string endpoint) {
    connected.insert(new ServerNode(pid, endpoint));
}

ServerNode *ConsistentHashing::get(const std::string &str) {
    auto it = connected.lower_bound(str);
    if (it == connected.end()) {
        it = connected.begin();
    }

    return *it;
}
