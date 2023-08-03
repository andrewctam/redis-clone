#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <map>

#include "entries/base_entry.hpp"

namespace cmd {
    std::string extract_name(const std::string& str);
    std::string extract_key(const std::string& str);

    bool addAll(const std::string& str);
    bool concatAll(const std::string& str);
    bool askAll(const std::string& str);

    enum class NodeCMDType {
        Not,
        Nodes,
        Create,
        Kill
    };

    NodeCMDType nodeCmds(const std::string& str);

}

class Command {
private:
    std::vector<std::string> args;

    std::string echo();
    std::string ping();
    std::string monitor();
    std::string shutdown();
    std::string keys();
    std::string benchmark();
    std::string flushall();
    std::string get();
    std::string set();
    std::string rename();
    std::string del();
    std::string exists();
    std::string dbsize();
    std::string type();
    std::string expire();
    std::string expireat();
    std::string persist();
    std::string incrementer();
    std::string list_push();
    std::string list_pop();
    std::string lrange();
    std::string llen();
    std::string hash();

    std::map<
        std::string, 
        std::function<const std::string()>
    > cmdMap = {
        {"echo", std::bind(&Command::echo, this)},
        {"ping", std::bind(&Command::ping, this)},
        {"monitor", std::bind(&Command::monitor, this)},
        {"shutdown", std::bind(&Command::shutdown, this)},
        {"keys", std::bind(&Command::keys, this)},
        {"benchmark", std::bind(&Command::benchmark, this)},
        {"flushall", std::bind(&Command::flushall, this)},
        {"get", std::bind(&Command::get, this)},
        {"set", std::bind(&Command::set, this)},
        {"rename", std::bind(&Command::rename, this)},
        {"del", std::bind(&Command::del, this)},
        {"exists", std::bind(&Command::exists, this)},
        {"dbsize", std::bind(&Command::dbsize, this)},
        {"type", std::bind(&Command::type, this)},
        {"expire", std::bind(&Command::expire, this)},
        {"expireat", std::bind(&Command::expireat, this)},
        {"persist", std::bind(&Command::persist, this)},
        {"incr", std::bind(&Command::incrementer, this)},
        {"incrby", std::bind(&Command::incrementer, this)},
        {"decr", std::bind(&Command::incrementer, this)},
        {"decrby", std::bind(&Command::incrementer, this)},
        {"lpush", std::bind(&Command::list_push, this)},
        {"rpush", std::bind(&Command::list_push, this)},
        {"lpop", std::bind(&Command::list_pop, this)},
        {"rpop", std::bind(&Command::list_pop, this)},
        {"lrange", std::bind(&Command::lrange, this)},
        {"llen", std::bind(&Command::llen, this)},
        {"hash", std::bind(&Command::hash, this)},

    };


public:
    Command(const std::string& str);
    std::string parse_cmd();
};


#endif
