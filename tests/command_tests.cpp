#include "gtest/gtest.h"

#include "command.h"
#include "../src/command.cpp"
TEST(CommandTests, EmptyCommand) {
    Command cmd { "" };
    std::string res = cmd.parse_cmd();

    EXPECT_EQ(res, "Invalid Command\n");
}

TEST(CommandTests, WhitespaceCommand) {
    Command cmd { " " };
    std::string res = cmd.parse_cmd();

    EXPECT_EQ(res, "Invalid Command\n");
}

TEST(CommandTests, UnknownCommand) {
    Command cmd { "notacommand 1 2 3" };
    std::string res = cmd.parse_cmd();

    EXPECT_EQ(res, "Invalid Command\n");
}

TEST(CommandTests, Echo) {
    Command cmd { " echo   1   23   45 6 7  8   9   " };
    std::string res = cmd.parse_cmd();

    EXPECT_EQ(res, "[echo 1 23 45 6 7 8 9]\n");
}
