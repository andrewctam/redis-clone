#include "command.h"
#include "hashmap.h"
#include "server.h"


Command::Command(const std::string& str, bool admin): admin(admin) {
    if (monitoring) {
        std::cerr << str << std::endl; 
    }

    std::stringstream ss (str);
    std::istream_iterator<std::string> begin (ss), end;
    args = std::vector<std::string> (begin, end);
};

std::string Command::parse_cmd() {

    if (args.size() == 0) {
        return "No Command Entered\n";
    }

    auto it = cmdMap.find(args[0]);
    if (it != cmdMap.end()) {
        return it->second();
    } else {
        return "Invalid Command\n";
    }
}

std::string Command::echo() {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            ss << " ";
        }

        ss << args[i];
    }
    ss << "]\n";

    return ss.str();
}

std::string Command::monitor() {
    if (!admin) {
        return "DENIED\n";
    }

    monitoring = !monitoring;

    return monitoring ? "ACTIVE\n" : "INACTIVE\n";
}

std::string Command::shutdown() {
    if (!admin) {
        return "DENIED\n";
    }

    stop = true;
    return "Stopping server...\n";
}

std::string Command::get() {
    if (args.size() < 2) {
        return "(NIL)\n";
    }

    BaseEntry *entry = hashmap.get(args[1]);
    if (!entry) {
        return "(NIL)\n";
    }

    switch(entry->get_type()) {            
        case ValueType::str:
            return dynamic_cast<StringEntry *>(entry)->value + "\n";
        default:
            return "NOT A STRING\n";
    }
}

std::string Command::set() {
    if (args.size() < 3) {
        return "FAILURE\n";
    }

    hashmap.add(args[1], new StringEntry(args[2]));
    return "SUCCESS\n";
}

std::string Command::del() {
    int count = 0;
    for (int i = 1; i < args.size(); i++) {
        if (hashmap.remove(args[i])) {
            count++;
        }
    }

    return std::to_string(count) + "\n";
}

std::string Command::exists() {    
    int count = 0;
    for (int i = 1; i < args.size(); i++) {
        if (hashmap.get(args[i])) {
            count++;
        }
    }

    return std::to_string(count) + "\n";
}