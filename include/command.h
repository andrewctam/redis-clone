#ifndef COMMAND_H
#define COMMAND_H
#include <iterator>
#include <vector>
#include <sstream>
#include <string>
#include <map>
#include <functional>


class Command {
private:
    std::vector<std::string> args;

    std::string echo();

    std::map<
        std::string, 
        std::function<const std::string()>
    > cmdMap = {
        {"echo", std::bind(&Command::echo, this)},
    };

public:
    Command(const std::string& str);
    std::string parse_cmd();
};
    
#endif
