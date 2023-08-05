#include "gtest/gtest.h"
#include <zmq.hpp>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <errno.h>

#include "consistent-hashing.hpp"
#include "worker.hpp"
#include "leader.hpp"

constexpr int END_OF_LINE = -1;

int client_stdin;
FILE *client_stdout;
FILE *client_stderr;

// read a line from the fd
// returns an empty string if equal
// returns the read string if not equal
std::string check_str(FILE *file, int num) {
    int MAX_LEN = 512;
    char buf[MAX_LEN];
    
    if (num == -1) {
        num = MAX_LEN - 1;
    }

    if (num >= MAX_LEN) {
        return "String too long. Increase MAX_LEN.";
    }

    fgets(buf, num, file);

    *(buf + num) = '\0';
    return std::string(buf);
}

int send_str(const char *str) {
    return write(client_stdin, str, strlen(str));
}

TEST(ServerTests, SetUp) {
    int in[2], out[2], err[2];
    EXPECT_TRUE(pipe(in) == 0);
    EXPECT_TRUE(pipe(out) == 0);
    EXPECT_TRUE(pipe(err) == 0);

    //start the server in another process, using the pipes and fds to communicate
    int ppid = getpid();
    int pid = fork(); 
    if (pid == 0) { //child
        if (prctl(PR_SET_PDEATHSIG, SIGINT) == -1 || getppid() != ppid) {
            exit(EXIT_FAILURE);
        }

        dup2(in[0], STDIN_FILENO);
        dup2(out[1], STDOUT_FILENO);
        dup2(err[1], STDERR_FILENO);

        close(in[1]);
        close(out[0]);
        close(err[0]);

        chdir("programs");
        execlp("./server", "./server", "-c", "16789", "-i", "26789", nullptr);
        FAIL();
        exit(EXIT_FAILURE);
    } else {
        EXPECT_EQ(pid == -1, false);

        client_stdin = in[1];
        client_stdout = fdopen(out[0], "r");
        client_stderr = fdopen(err[0], "r");

        close(in[0]);
        close(out[1]);
        close(err[1]);
    }
}

TEST(ServerTests, StartUp) {
    std::string leader_start = "Started leader node with pid";
    std::string worker_start = "Connected to worker node with pid:";

    int leader_count = 0;
    int worker_count = 0;

    for (int i = 0; i < 4; i++) {
        std::string line = check_str(client_stderr, END_OF_LINE);

        if (line.compare(0, leader_start.size(), leader_start) == 0) {
            leader_count++;
        } else if (line.compare(0, worker_start.size(), worker_start) == 0) {
            worker_count++;
        } else {
            std::cout << line << std::endl;
            FAIL();
        }
    }

    EXPECT_EQ(leader_count, 1);
    EXPECT_EQ(worker_count, 3);

    EXPECT_EQ(check_str(client_stdout, END_OF_LINE), "Client started!\n");
    EXPECT_EQ(check_str(client_stdout, 3), "> ");
}

TEST(ServerTests, BasicCache) {    
    EXPECT_GT(send_str("set a 1\n"), 0);
    EXPECT_EQ(check_str(client_stdout, END_OF_LINE), "SUCCESS\n");
    EXPECT_EQ(check_str(client_stdout, 3), "> ");

    EXPECT_GT(send_str("get a\n"), 0);
    EXPECT_EQ(check_str(client_stdout, END_OF_LINE), "1\n");
    EXPECT_EQ(check_str(client_stdout, 3), "> ");
}
