#include "command.h"
#include "hashmap.h"
#include "hashmap.cpp"


Command::Command(const std::string& str) {
    std::stringstream ss (str);
    std::istream_iterator<std::string> begin (ss), end;
    args = std::vector<std::string> (begin, end);
};

std::string Command::parse_cmd() {
    static std::string error = "Invalid Command\n";

    if (args.size() == 0) {
        return error;
    }

    auto it = cmdMap.find(args[0]);
    if (it != cmdMap.end()) {
        return it->second();
    } else {
        return error;
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



