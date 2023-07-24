#include "command.h"
#include "lru_cache.h"
#include "server.h"
#include "unix_times.h"
#include "entries/list_entry.h"

Command::Command(const std::string& str, bool admin): admin(admin) {
    if (monitoring) {
        std::cerr << str << std::endl; 
    }

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
        return "No Command Entered\n";
    }

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    auto it = cmdMap.find(cmd);
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


std::string Command::keys() {
    if (!admin) {
        return "DENIED\n";
    }
    
    std::vector<std::string> keys = cache.key_set(true);

    if (keys.size() == 0) {
        return "ERROR\n";
    }

    return keys[0] + "\n";
}

std::string Command::benchmark() {
    if (!admin) {
        return "DENIED\n";
    }

    long num = 0;
    try {
        if (args.size() < 2) {
            throw "No num provided";
        }

        num = stol(args[1]);
    } catch(...) {
        return "FAILURE\n";
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
    return std::to_string(time_taken) + " ms\n";
}

std::string Command::get() {
    if (args.size() < 2) {
        return "(NIL)\n";
    }

    BaseEntry *entry = cache.get(args[1]);
    if (!entry) {
        return "(NIL)\n";
    }

    return entry->to_string() + "\n";
}

std::string Command::set() {
    if (args.size() < 3) {
        return "FAILURE\n";
    }

    cache.add(args[1], str_to_base_entry(args[2]));

    return "SUCCESS\n";
}

std::string Command::del() {
    int count = 0;
    for (int i = 1; i < args.size(); i++) {
        if (cache.remove(args[i])) {
            count++;
        }
    }

    return std::to_string(count) + "\n";
}

std::string Command::exists() {    
    int count = 0;
    for (int i = 1; i < args.size(); i++) {
        if (cache.get(args[i])) {
            count++;
        }
    }

    return std::to_string(count) + "\n";
}

std::string Command::dbsize() {    
    return std::to_string(cache.size()) + "\n";
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
        return "FAILURE\n";
    }
    
    bool res = cache.set_expire(args[1], time);
    return res ? std::to_string(time) + "\n": "FAILURE\n";
}

std::string Command::expireat() {
    try {
        if (args.size() < 3) {
            throw "No args provided";
        }

        uint64_t unix_times = std::stol(args[2]);

        bool res = cache.set_expire(args[1], unix_times);
        return res ? "SUCCESS\n" : "FAILURE\n";
    } catch(...) {
        return "FAILURE\n";
    }
}


std::string Command::persist() {
    seconds::rep time = time_secs();
    
    if (args.size() < 2) {
        throw "No args provided";
    }

    bool res = cache.set_expire(args[1], 0);
    return res ? "SUCCESS\n": "FAILURE\n";
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
        return "FAILURE\n";
    }

    if (args[0] == "decr" || args[0] == "decrby") {
        change *= -1;
    }

    BaseEntry *entry = cache.get(args[1]);
    if (!entry) {
        cache.add(args[1], new IntEntry(change));
        return std::to_string(change) + "\n";
    }

    if (entry->get_type() != EntryType::integer) {
        return "NOT AN INT\n";
    }

    IntEntry *val = dynamic_cast<IntEntry *>(entry);
    val->value += change;

    return std::to_string(val->value) + "\n";
}


std::string Command::list_push() {
    if (args.size() < 3) {
        return "FAILURE\n";
    }

    BaseEntry *entry = cache.get(args[1]);
    LinkedList *list;
    
    ListEntry *list_entry;
    if (!entry) {
        list_entry = new ListEntry();
        cache.add(args[1], list_entry);
    } else if (entry->get_type() != EntryType::list) {
        return "NOT A LIST\n";
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

    return std::to_string(list->get_size()) + "\n";
}


std::string Command::list_pop() {
    if (args.size() < 2) {
        return "FAILURE\n";
    }
    int num = 1;
    if (args.size() > 2) {
        try {
            num = stoi(args[2]);

            if (num == 0) {
                return "[]\n";
            } else if (num < 0) {
                return "ERROR\n";
            }
        } catch (...) {
            num = 1;
        }
    }

    BaseEntry *entry = cache.get(args[1]);
    
    if (!entry) {
        return "(NIL)\n";
    } 

    if (entry->get_type() != EntryType::list) {
        return "NOT A LIST\n";
    }

    ListEntry *list_entry = dynamic_cast<ListEntry*>(entry);
    LinkedList *list = list_entry->list;
    bool rpop = args[0] == "rpop";

    std::stringstream ss;
    ss << "[";

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

    ss << "]\n";
    return ss.str();
}

std::string Command::lrange() {
    if (args.size() < 2) {
        return "FAILURE\n";
    }
    int start = 0;
    int stop = -1;
    if (args.size() > 2) {
        try {
            start = stoi(args[2]);

            if (start < -1) {
                return "ERROR\n";
            }
        } catch (...) {
            start = 0;
        }
    }

    if (args.size() > 3) {
        try {
            stop = stoi(args[3]);

            if (stop < -1) {
                return "ERROR\n";
            }
        } catch (...) {
            stop = -1;
        }
    }

    BaseEntry *entry = cache.get(args[1]);
    
    if (!entry) {
        return "(NIL)\n";
    } 

    if (entry->get_type() != EntryType::list) {
        return "NOT A LIST\n";
    }

    ListEntry *list_entry = dynamic_cast<ListEntry*>(entry);
    LinkedList *list = list_entry->list;

    int lim = list->get_size() - 1;
    stop = std::min(stop, lim);

    if (start > lim) {
        return "[]\n";
    }

    if (start == -1) {
        start = lim;
    }
    if (stop == -1) {
        stop = lim;
    }

    std::vector<std::string> str = list->values(start, stop, false, true);

    if (str.size() == 0) {
        return "ERROR\n";
    }

    return str[0] + "\n";
}

std::string Command::llen() {
    if (args.size() < 2) {
        return "FAILURE\n";
    }

    BaseEntry *entry = cache.get(args[1]);
    
    if (!entry) {
        return "0\n";
    } 

    if (entry->get_type() != EntryType::list) {
        return "NOT A LIST\n";
    }

    ListEntry *list_entry = dynamic_cast<ListEntry*>(entry);
    LinkedList *list = list_entry->list;

    return std::to_string(list->get_size()) + "\n";
}
