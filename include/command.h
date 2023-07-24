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
    bool admin;

    std::string echo();
    std::string monitor();
    std::string shutdown();
    std::string keys();
    std::string benchmark();
    std::string get();
    std::string set();
    std::string del();
    std::string exists();
    std::string dbsize();
    std::string expire();
    std::string expireat();
    std::string persist();
    std::string incrementer();


    std::map<
        std::string, 
        std::function<const std::string()>
    > cmdMap = {
        {"echo", std::bind(&Command::echo, this)},
        {"monitor", std::bind(&Command::monitor, this)},
        {"shutdown", std::bind(&Command::shutdown, this)},
        {"keys", std::bind(&Command::keys, this)},
        {"benchmark", std::bind(&Command::benchmark, this)},
        {"get", std::bind(&Command::get, this)},
        {"set", std::bind(&Command::set, this)},
        {"del", std::bind(&Command::del, this)},
        {"exists", std::bind(&Command::exists, this)},
        {"dbsize", std::bind(&Command::dbsize, this)},
        {"expire", std::bind(&Command::expire, this)},
        {"expireat", std::bind(&Command::expireat, this)},
        {"persist", std::bind(&Command::persist, this)},
        {"incr", std::bind(&Command::incrementer, this)},
        {"incrby", std::bind(&Command::incrementer, this)},
        {"decr", std::bind(&Command::incrementer, this)},
        {"decrby", std::bind(&Command::incrementer, this)},
    };

public:
    Command(const std::string& str, bool admin = false);
    std::string parse_cmd();
};
    
#endif
