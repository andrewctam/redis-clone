#include <iterator>
#include <sstream>
#include <unordered_set>
#include <functional>

#include "command.hpp"
#include "lru_cache.hpp"
#include "globals.hpp"
#include "unix_times.hpp"
#include "entries/list_entry.hpp"

namespace cmd {
    std::string extract_name(const std::string& str) {
        std::istringstream iss(str);
        std::istream_iterator<std::string> it(iss);
        if (it != std::istream_iterator<std::string>() && !(*it).empty()) {
            return *it;
        }

        return "";
    }
    std::string extract_key(const std::string& str) {
        std::istringstream iss(str);
        std::istream_iterator<std::string> it(iss);
        
        ++it;

        std::string key = "";

        if (it != std::istream_iterator<std::string>() && !(*it).empty()) {
            key = *it;
        }

        return key;
    }
    bool addAll(const std::string& str) {
        std::unordered_set<std::string> cmds = {
            "dbsize",
            "exists"
        };
        return cmds.find(str) != cmds.end();
    }

    bool concatAll(const std::string& str) {
         std::unordered_set<std::string> cmds = {
            "keys"
        };
        return cmds.find(str) != cmds.end();
    }

    bool askAll(const std::string& str) {
        std::unordered_set<std::string> cmds = {
            "flushall",
            "shutdown"
        };
        return cmds.find(str) != cmds.end();
    }

    bool nodeCmds(const std::string& str) {
         std::unordered_set<std::string> cmds = {
            "nodes",
            "create",
            "kill"
        };
        return cmds.find(str) != cmds.end();
    }
}

Command::Command(const std::string& str) {
    std::stringstream ss (str);
    std::istream_iterator<std::string> begin (ss), end;
    args = std::vector<std::string> (begin, end);
};

BaseEntry *Command::str_to_base_entry(std::string str) {
    try {
        for(char& ch : str) {
            if (ch < '0' || ch > '9') {
                throw "Not an int";
            }
        }
        
        int intVal = std::stoi(str);
        return new IntEntry(intVal);
    } catch(...) {
        return new StringEntry(str);
    }
}

std::string Command::parse_cmd() {
    if (args.size() == 0) {
        return "No Command Entered";
    }

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    auto it = cmdMap.find(cmd);
    if (it != cmdMap.end()) {
        return it->second();
    } else {
        return "Invalid Command";
    }
}



std::string Command::echo() {
    std::stringstream ss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            ss << " ";
        }

        ss << args[i];
    }

    return ss.str();
}

std::string Command::ping() {
    return "PONG";
}

std::string Command::monitor() {
    monitoring = !monitoring;
    return monitoring ? "ACTIVE" : "INACTIVE";
}

std::string Command::shutdown() {
    stop = true;
    return "Stopping server...";
}


std::string Command::keys() {
    std::vector<std::string> keys = cache.key_set(true);
    return keys[0];
}

std::string Command::benchmark() {
    long num = 0;
    try {
        if (args.size() < 2) {
            throw "No num provided";
        }

        num = stol(args[1]);
    } catch(...) {
        return "FAILURE";
    }
    
    std::srand(std::time(nullptr));

    milliseconds::rep start = time_ms();

    const int key_limit = 1000;
    for (long i = 0; i < num; i++) {
        if(i % 2) {
            Command set { "set " + std::to_string(std::rand() % key_limit) + " " + std::to_string(std::rand()) };
            set.parse_cmd();
        } else {
            Command get { "get " + std::to_string(std::rand() % key_limit)};
            get.parse_cmd();
        }
    }
    milliseconds::rep time_taken = time_ms() - start;
    return std::to_string(time_taken) + " ms";
}


std::string Command::flushall() {
    cache.clear();
    return "Cache cleared";
}

std::string Command::get() {
    if (args.size() < 2) {
        return "(NIL)";
    }

    BaseEntry *entry = cache.get(args[1]);
    if (!entry) {
        return "(NIL)";
    }

    return entry->to_string() + "";
}

std::string Command::set() {
    if (args.size() < 3) {
        return "FAILURE";
    }

    cache.add(args[1], str_to_base_entry(args[2]));

    return "SUCCESS";
}


std::string Command::rename() {
    if (args.size() < 3) {
        return "FAILURE";
    }

    BaseEntry *entry = cache.remove(args[1]);
    if (!entry) {
        return "FAILURE";
    }

    cache.add(args[2], entry);
    return "SUCCESS";
}

std::string Command::del() {
    int count = 0;
    for (int i = 1; i < args.size(); i++) {
        BaseEntry *rem = cache.remove(args[i]);
        if (rem) {
            delete rem;
            count++;
        }
    }

    return std::to_string(count) + "";
}

std::string Command::exists() {    
    int count = 0;
    for (int i = 1; i < args.size(); i++) {
        if (cache.get(args[i])) {
            count++;
        }
    }

    return std::to_string(count) + "";
}

std::string Command::dbsize() {    
    return std::to_string(cache.size()) + "";
}

std::string Command::type() {    
    if (args.size() < 2) {
        return "FAILURE";
    }

    BaseEntry *entry = cache.get(args[1]);
    if (!entry) {
        return "(NIL)";
    }

    switch (entry->get_type()) {
        case EntryType::str: return "string";
        case EntryType::integer: return "int";
        case EntryType::list: return "list";
        default: return "?";
    }
}

std::string Command::expire() {
    seconds::rep time = time_secs();
    
    long secs = 0;
    try {
        if (args.size() < 3) {
            throw "No args provided";
        }

        time += std::stol(args[2]);
    } catch(...) {
        return "FAILURE";
    }
    
    bool res = cache.set_expire(args[1], time);
    return res ? std::to_string(time) + "": "FAILURE";
}

std::string Command::expireat() {
    try {
        if (args.size() < 3) {
            throw "No args provided";
        }

        uint64_t unix_times = std::stol(args[2]);

        bool res = cache.set_expire(args[1], unix_times);
        return res ? "SUCCESS" : "FAILURE";
    } catch(...) {
        return "FAILURE";
    }
}


std::string Command::persist() {
    seconds::rep time = time_secs();
    
    if (args.size() < 2) {
        throw "No args provided";
    }

    bool res = cache.set_expire(args[1], 0);
    return res ? "SUCCESS": "FAILURE";
}


std::string Command::incrementer() {
    int change = 1;
    try {
        if (args.size() < 2) {
            throw "No key provided";
        }

        if ((args[0] == "incrby" || args[0] == "decrby") && args.size() < 3) {
            throw "No amount provided";
        }

        if (args[0] == "incrby" || args[0] == "decrby") {
            change = std::stoi(args[2]);
        }

    } catch (...) {
        return "FAILURE";
    }

    if (args[0] == "decr" || args[0] == "decrby") {
        change *= -1;
    }

    BaseEntry *entry = cache.get(args[1]);
    if (!entry) {
        cache.add(args[1], new IntEntry(change));
        return std::to_string(change) + "";
    }

    if (entry->get_type() != EntryType::integer) {
        return "NOT AN INT";
    }

    IntEntry *val = dynamic_cast<IntEntry *>(entry);
    val->value += change;

    return std::to_string(val->value) + "";
}


std::string Command::list_push() {
    if (args.size() < 3) {
        return "FAILURE";
    }

    BaseEntry *entry = cache.get(args[1]);
    LinkedList *list;
    
    ListEntry *list_entry;
    if (!entry) {
        list_entry = new ListEntry();
        cache.add(args[1], list_entry);
    } else if (entry->get_type() != EntryType::list) {
        return "NOT A LIST";
    } else {
        list_entry = dynamic_cast<ListEntry*>(entry);
    }

    list = list_entry->list;
    bool lpush = args[0] == "lpush";
    
    for (int i = 2; i < args.size(); i++) {
        if (lpush) {
            list->add_front(str_to_base_entry(args[i]));
        } else {
            list->add_end(str_to_base_entry(args[i]));
        }
    }

    return std::to_string(list->get_size()) + "";
}


std::string Command::list_pop() {
    if (args.size() < 2) {
        return "FAILURE";
    }
    int num = 1;
    if (args.size() > 2) {
        try {
            num = stoi(args[2]);

            if (num == 0) {
                return "";
            } else if (num < 0) {
                return "ERROR";
            }
        } catch (...) {
            num = 1;
        }
    }

    BaseEntry *entry = cache.get(args[1]);
    
    if (!entry) {
        return "(NIL)";
    } 

    if (entry->get_type() != EntryType::list) {
        return "NOT A LIST";
    }

    ListEntry *list_entry = dynamic_cast<ListEntry*>(entry);
    LinkedList *list = list_entry->list;
    bool rpop = args[0] == "rpop";

    std::stringstream ss;

    num = std::min(num, list->get_size());
    bool first_entry = true;
    while (num > 0) {
        if (!first_entry) {
            ss << " ";
        } else {
            first_entry = false;
        }
        
        BaseEntry* rem;
        if (rpop) {
            rem = list->remove_end();
        } else {
            rem = list->remove_front();
        }

        ss << rem->to_string();
        num--;

        delete rem;
    }

    return ss.str();
}

std::string Command::lrange() {
    if (args.size() < 2) {
        return "FAILURE";
    }
    int start = 0;
    int stop = -1;
    if (args.size() > 2) {
        try {
            start = stoi(args[2]);

            if (start < -1) {
                return "ERROR";
            }
        } catch (...) {
            start = 0;
        }
    }

    if (args.size() > 3) {
        try {
            stop = stoi(args[3]);

            if (stop < -1) {
                return "ERROR";
            }
        } catch (...) {
            stop = -1;
        }
    }

    BaseEntry *entry = cache.get(args[1]);
    
    if (!entry) {
        return "(NIL)";
    } 

    if (entry->get_type() != EntryType::list) {
        return "NOT A LIST";
    }

    ListEntry *list_entry = dynamic_cast<ListEntry*>(entry);
    LinkedList *list = list_entry->list;

    int lim = list->get_size() - 1;
    stop = std::min(stop, lim);

    if (start > lim) {
        return "";
    }

    if (start == -1) {
        start = lim;
    }
    if (stop == -1) {
        stop = lim;
    }

    std::vector<std::string> str = list->values(start, stop, false, true);

    if (str.size() == 0) {
        return "ERROR";
    }

    return str[0];
}

std::string Command::llen() {
    if (args.size() < 2) {
        return "FAILURE";
    }

    BaseEntry *entry = cache.get(args[1]);
    
    if (!entry) {
        return "0";
    } 

    if (entry->get_type() != EntryType::list) {
        return "NOT A LIST";
    }

    ListEntry *list_entry = dynamic_cast<ListEntry*>(entry);
    LinkedList *list = list_entry->list;

    return std::to_string(list->get_size()) + "";
}
