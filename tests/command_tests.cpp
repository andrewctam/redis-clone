#include "gtest/gtest.h"

#include "command.h"
#include "server.h"
#include "unix_secs.h"

class CommandTests: public ::testing::Test {
protected:
    void SetUp() override {
        // Reset global variables before each test case
        monitoring = false;
        stop = false;
        hashmap = HashMap(5);
        time_offset = 0;
    }
};


TEST_F(CommandTests, EmptyCommand) {
    Command cmd { "" };
    EXPECT_EQ(cmd.parse_cmd(), "No Command Entered\n");
}

TEST_F(CommandTests, WhitespaceCommand) {
    Command cmd { " " };
    EXPECT_EQ(cmd.parse_cmd(), "No Command Entered\n");
}

TEST_F(CommandTests, UnknownCommand) {
    Command cmd { "notacommand 1 2 3" };
    EXPECT_EQ(cmd.parse_cmd(), "Invalid Command\n");
}

TEST_F(CommandTests, Echo) {
    Command cmd { " echo   1   23   45 6 7  8   9   " };
    EXPECT_EQ(cmd.parse_cmd(), "[echo 1 23 45 6 7 8 9]\n");
}

TEST_F(CommandTests, Monitor) {
    Command cmd_non_admin { "monitor", false };
    EXPECT_EQ(cmd_non_admin.parse_cmd(), "DENIED\n");
    EXPECT_EQ(monitoring, false);

    Command cmd_admin { "monitor", true };
    EXPECT_EQ(cmd_admin.parse_cmd(), "ACTIVE\n");
    EXPECT_EQ(monitoring, true);

    EXPECT_EQ(cmd_admin.parse_cmd(), "INACTIVE\n");
    EXPECT_EQ(monitoring, false);
}

TEST_F(CommandTests, Shutdown) {
    Command cmd_non_admin { "shutdown", false };
    EXPECT_EQ(cmd_non_admin.parse_cmd(), "DENIED\n");
    EXPECT_EQ(stop, false);

    Command cmd_admin { "shutdown", true };
    EXPECT_EQ(cmd_admin.parse_cmd(), "Stopping server...\n");
    EXPECT_EQ(stop, true);
}

TEST_F(CommandTests, GetSet) {
    Command get_dne { "get a" };
    EXPECT_EQ(get_dne.parse_cmd(), "(NIL)\n");

    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS\n");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1\n");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS\n");

    Command get_b { "get b" };
    EXPECT_EQ(get_b.parse_cmd(), "2\n");

    Command overwrite_a { "set a 3"};
    EXPECT_EQ(overwrite_a.parse_cmd(), "SUCCESS\n");

    Command get_new_a { "get a" };
    EXPECT_EQ(get_new_a.parse_cmd(), "3\n");
}


TEST_F(CommandTests, Del) {
    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS\n");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS\n");

    Command set_c { "set c 3" };
    EXPECT_EQ(set_c.parse_cmd(), "SUCCESS\n");

    Command remove_none { "del "};
    EXPECT_EQ(remove_none.parse_cmd(), "0\n");

    Command remove_some { "del a b"};
    EXPECT_EQ(remove_some.parse_cmd(), "2\n");

    Command remove_dne { "del a b"};
    EXPECT_EQ(remove_dne.parse_cmd(), "0\n");

    Command remove_randoms { "del c d a e"};
    EXPECT_EQ(remove_randoms.parse_cmd(), "1\n");
}

TEST_F(CommandTests, Exists) {
    Command exists_empty { "exists a b c" };
    EXPECT_EQ(exists_empty.parse_cmd(), "0\n");

    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS\n");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS\n");

    Command set_c { "set c 3" };
    EXPECT_EQ(set_c.parse_cmd(), "SUCCESS\n");

    Command exists_none { "exists" };
    EXPECT_EQ(exists_none.parse_cmd(), "0\n");

    Command exists_all { "exists a b c" };
    EXPECT_EQ(exists_all.parse_cmd(), "3\n");

    Command exists_some { "exists a c" };
    EXPECT_EQ(exists_some.parse_cmd(), "2\n");

    Command exists_not_included { "exists a z y 7" };
    EXPECT_EQ(exists_not_included.parse_cmd(), "1\n");
}

TEST_F(CommandTests, Expire) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS\n");

    Command expire { "expire a 50" };

    std::string res = expire.parse_cmd();
    std::string time = std::to_string(time_secs() + 50) + "\n";
    //in case it is at the end of the second
    std::string close = std::to_string(time_secs() + 51) + "\n";
    EXPECT_TRUE(res == time || res == close);

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1\n");

    //offset the time_secs() function;
    time_offset = 100;

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "(NIL)\n");
}

TEST_F(CommandTests, ExpireAt) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS\n");

    Command expire { "expireat a " + std::to_string(time_secs() + 50)};
    EXPECT_EQ(expire.parse_cmd(), "SUCCESS\n");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1\n");

    //offset the time_secs() function;
    time_offset = 100;

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "(NIL)\n");

}