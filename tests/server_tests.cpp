#include "gtest/gtest.h"
#include <zmq.hpp>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <errno.h>
#include <vector>
#include <iostream>
#include <string>
#include <random>

#include "consistent-hashing.hpp"
#include "worker.hpp"
#include "leader.hpp"

constexpr int END_OF_LINE = -1;

int client_stdin;
FILE *client_stdout;

// read num chars from stdout, or until end of line if -1
std::string check_stdout(int num) {
    int MAX_LEN = 512;
    char buf[MAX_LEN];
    
    if (num == -1) {
        num = MAX_LEN - 1;
    }

    if (num >= MAX_LEN) {
        return "String too long. Increase MAX_LEN.";
    }

    fgets(buf, num, client_stdout);

    *(buf + num) = '\0';
    return std::string(buf);
}

// compares a string to a substring
bool check_substr(const std::string &line, const std::string &sub, bool print_err = true) {
    bool res = line.compare(0, sub.size(), sub) == 0;
    if (print_err && !res) {
        std::cout << "Strings did not match!\nGot: " << line << ". Exp: " << sub << std::endl;
    }
    return res;
}

// reads a line frm stdout and compares it to a substring
bool check_substr_stdout(const std::string &sub, bool print_err = true) {
    std::string line = check_stdout(END_OF_LINE);
    return check_substr(line, sub, print_err);
}

// keeps reading lines from stdout until a match is found
void read_until_substr(const std::string &sub, bool print_err = false) {
    while (true) {
        std::string line = check_stdout(END_OF_LINE);
        if (check_substr(line, sub, print_err)) {
            return;
        }
    }
}

// sends a string to stdin
int send_str(const char *str) {
    return write(client_stdin, str, strlen(str));
}


std::string randomKey() {
    int minLength = 2;
    int maxLength = 10;

    const std::string charSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> lengthDistribution(minLength, maxLength);
    std::uniform_int_distribution<> charDistribution(0, charSet.size() - 1);

    int length = lengthDistribution(gen);

    std::string result;
    for (int i = 0; i < length; ++i) {
        result += charSet[charDistribution(gen)];
    }

    return result;
}

TEST(ServerTests, SetUp) {
    int in[2], out[2];
    EXPECT_TRUE(pipe(in) == 0);
    EXPECT_TRUE(pipe(out) == 0);

    //start the server in another process, using the pipes and fds to communicate
    int ppid = getpid();
    int pid = fork(); 
    if (pid == 0) { //child
        if (prctl(PR_SET_PDEATHSIG, SIGINT) == -1 || getppid() != ppid) {
            exit(EXIT_FAILURE);
        }

        dup2(in[0], STDIN_FILENO);
        dup2(out[1], STDOUT_FILENO);

        close(in[1]);
        close(out[0]);

        chdir("programs");
        execlp("./server", "./server", "-c", "16789", "-i", "26789", nullptr);
        FAIL();
        exit(EXIT_FAILURE);
    } else {
        EXPECT_EQ(pid == -1, false);

        client_stdin = in[1];
        client_stdout = fdopen(out[0], "r");

        close(in[0]);
        close(out[1]);
    }
}

TEST(ServerTests, StartUp) {
    int leader_count = 0;
    int worker_count = 0;
    int client_count = 0;

    for (int i = 0; i < 5; i++) {
        std::string line = check_stdout(END_OF_LINE);
        if (check_substr(line, "Started leader node with pid", false)) {
            leader_count++;
        } else if (check_substr(line, "Connected to worker node with pid", false)) {
            worker_count++;
        } else if (line == "Client started!\n") {
            EXPECT_EQ(check_stdout(3), "> ");
            client_count++;
        }
    }

    EXPECT_EQ(leader_count, 1);
    EXPECT_EQ(worker_count, 3);
    EXPECT_EQ(client_count, 1);
}

TEST(ServerTests, BasicCache) {    
    EXPECT_GT(send_str("set a 1\n"), 0);
    EXPECT_EQ(check_stdout(END_OF_LINE), "SUCCESS\n");
    EXPECT_EQ(check_stdout(3), "> ");

    EXPECT_GT(send_str("get a\n"), 0);
    EXPECT_EQ(check_stdout(END_OF_LINE), "1\n");
    EXPECT_EQ(check_stdout(3), "> ");
}


TEST(ServerTests, DistributedRelocation) {
    std::vector<std::string> keys;
    keys.emplace_back("a");

    for (int i = 0; i < 500; i++) {
        std::string key = randomKey();
        keys.emplace_back(key);

        std::string cmd = "set " + key + " 1\n";

        EXPECT_GT(send_str(cmd.c_str()), 0);
        EXPECT_EQ(check_stdout(END_OF_LINE), "SUCCESS\n");
        EXPECT_EQ(check_stdout(3), "> ");
    }

    // create a node to redistribute cache
    EXPECT_GT(send_str("create\n"), 0);

    EXPECT_TRUE(check_substr_stdout("Worker node created with pid"));
    EXPECT_EQ(check_stdout(3), "> ");

    EXPECT_TRUE(check_substr_stdout("Connected to worker node with pid:"));

    
    int count = 0;
    for (auto &key : keys) {
        std::string cmd = "get " + key + "\n";

        EXPECT_GT(send_str(cmd.c_str()), 0);
        std::string val = check_stdout(END_OF_LINE);
        if (val == "1\n") {
            count++;
        } else {
            std::cout << key  << " ["  << hash_function(key) << "] " << val << std::endl;
        }
        EXPECT_EQ(check_stdout(3), "> ");
    }
    
    EXPECT_EQ(count, keys.size());
    //print nodes for debugging purposes
    if (count != keys.size()) {
        send_str("nodes\n");
        for (int i = 0; i < 5; i++) {
            std::cout << check_stdout(END_OF_LINE);
        }
        check_stdout(3);

        send_str("dist\n");
        std::cout << check_stdout(END_OF_LINE);
        check_stdout(3);

        std::flush(std::cout);
    }
    
}


TEST(ServerTests, LeaderElections) {
    for (int i = 0; i < 3; i++) {
        send_str("kill leader\n");
        read_until_substr("Started leader"); 
        sleep(1);
        
        EXPECT_GT(send_str("set a 1\n"), 0);
        read_until_substr("SUCCESS");
        EXPECT_EQ(check_stdout(3), "> ");

        EXPECT_GT(send_str("get a\n"), 0);
        EXPECT_EQ(check_stdout(END_OF_LINE), "1\n");
        EXPECT_EQ(check_stdout(3), "> ");
    }

    send_str("kill leader\n");
    EXPECT_EQ(check_stdout(3), "OK");    
}
