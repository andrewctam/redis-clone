#include "consistent-hashing.h"

ServerNode::ServerNode(std::string pid, std::string endpoint) :
    pid(pid), 
    context(new zmq::context_t(1)),
    socket(new zmq::socket_t(*context, zmq::socket_type::req)),
    hash(std::hash<std::string>()(pid + endpoint) % 360) {

    socket->connect(endpoint);
}
ServerNode::ServerNode(std::string str) :
    pid(""), 
    context(nullptr),
    socket(nullptr),
    hash(std::hash<std::string>()(str) % 360) { }



ServerNode::~ServerNode() {
    if (socket)
        delete socket;
    if (context)
        delete context;
}




void ConsistentHashing::add(std::string pid, std::string endpoint) {
    connected.insert(new ServerNode(pid, endpoint));
}

ServerNode *ConsistentHashing::get(const std::string &str) {
    ServerNode *temp = new ServerNode(str);
    auto it = connected.lower_bound(temp);
    if (it == connected.end()) {
        it = connected.begin();
    }

    delete temp;
    return *it;
}
