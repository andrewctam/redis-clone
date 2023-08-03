#include "gtest/gtest.h"
#include <zmq.hpp>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <errno.h>

#include "consistent-hashing.hpp"
#include "worker.hpp"
#include "leader.hpp"

int client_stdin;
FILE *client_stdout;
FILE *client_stderr;

// read a line from the fd and compare it to exp
// returns an empty string if equal
// returns the read string if not equal
std::string check_str(FILE *file, char const *exp, bool end_of_line = true) {
    char buf[512];
    int len = strlen(exp);

    if (len >= 512) {
        return "String too long";
    }

    fgets(buf, end_of_line ? 512 : len + 1, file);
    if (strncmp(buf, exp, len) == 0) {
        return "";
    }

    *(buf + len) = '\0';
    return std::string(buf);
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
    usleep(1000 * 100);
    
    EXPECT_EQ(check_str(client_stderr, "Started server with leader node with pid"), "");
    EXPECT_EQ(check_str(client_stderr, "Connected to worker node with pid:"), "");
    EXPECT_EQ(check_str(client_stderr, "Connected to worker node with pid:"), "");
    EXPECT_EQ(check_str(client_stderr, "Connected to worker node with pid:"), "");
    EXPECT_EQ(check_str(client_stdout, "Client started!"), "");
    EXPECT_EQ(check_str(client_stdout, "> ", false), "");

}
