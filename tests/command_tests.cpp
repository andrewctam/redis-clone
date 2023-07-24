#include "gtest/gtest.h"

#include "command.h"
#include "server.h"
#include "unix_times.h"

class CommandTests: public ::testing::Test {
protected:
    void SetUp() override {
        // Reset global variables before each test case
        monitoring = false;
        stop = false;
        cache = LRUCache(5);
        secs_offset = 0;
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


TEST_F(CommandTests, Keys) {
    Command non_admin { "keys" };
    EXPECT_EQ(non_admin.parse_cmd(), "DENIED\n");

    Command keys_none { "keys", true };
    EXPECT_EQ(keys_none.parse_cmd(), "[]\n");

    Command set_a { "set a 1", true };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS\n");

    Command set_b { "set b 2", true };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS\n");

    Command set_C { "set c 3", true };
    EXPECT_EQ(set_C.parse_cmd(), "SUCCESS\n");

    Command keys_1 { "keys", true };
    EXPECT_EQ(keys_1.parse_cmd(), "[a b c]\n");

    Command del_b { "del b", true };
    EXPECT_EQ(del_b.parse_cmd(), "1\n");

    Command keys_2 { "keys", true };
    EXPECT_EQ(keys_2.parse_cmd(), "[a c]\n");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "2\n");
}

TEST_F(CommandTests, Benchmark) {
    Command non_admin { "benchmark 10", false };
    EXPECT_EQ(non_admin.parse_cmd(), "DENIED\n");

    Command benchmark { "benchmark 10", true };
    std::string run_time = benchmark.parse_cmd();

    EXPECT_TRUE(run_time == "0 ms\n" || "1 ms\n");
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

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "2\n");
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

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "0\n");
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

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "3\n");
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
    secs_offset = 100;

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1\n");

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "(NIL)\n");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "0\n");

    Command dbsize_rm { "dbsize" };
    EXPECT_EQ(dbsize_rm.parse_cmd(), "0\n");
}

TEST_F(CommandTests, ExpireAt) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS\n");

    Command expire { "expireat a " + std::to_string(time_secs() + 50)};
    EXPECT_EQ(expire.parse_cmd(), "SUCCESS\n");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1\n");

    //offset the time_secs() function;
    secs_offset = 100;

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "(NIL)\n");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "0\n");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "0\n");
}


TEST_F(CommandTests, Persist) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS\n");

    Command persist_fail { "persist a" };
    EXPECT_EQ(persist_fail.parse_cmd(), "FAILURE\n");

    Command expire { "expire a 50" };
    expire.parse_cmd();

    Command persist { "persist a" };
    EXPECT_EQ(persist.parse_cmd(), "SUCCESS\n");


    //offset the time_secs() function;
    secs_offset = 100;

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "1\n");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1\n");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "1\n");
}



TEST_F(CommandTests, IncrementersNotNum) {
    Command set_a { "set a notanum" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS\n");

    Command incr_fail { "incr a" };
    EXPECT_EQ(incr_fail.parse_cmd(), "NOT AN INT\n");

    Command incrby_fail { "incrby a 2" };
    EXPECT_EQ(incrby_fail.parse_cmd(), "NOT AN INT\n");

    Command decr_fail { "decr a" };
    EXPECT_EQ(decr_fail.parse_cmd(), "NOT AN INT\n");

    Command decrby_fail { "decrby a 2" };
    EXPECT_EQ(decrby_fail.parse_cmd(), "NOT AN INT\n");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1\n");
}

TEST_F(CommandTests, Incrementers) {
    Command set_a { "set a 0" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS\n");


    Command incr { "incr a" };
    EXPECT_EQ(incr.parse_cmd(), "1\n");
    Command get1 { "get a" };
    EXPECT_EQ(get1.parse_cmd(), "1\n");


    Command decr { "decr a" };
    EXPECT_EQ(decr.parse_cmd(), "0\n");
    Command get0 { "get a" };
    EXPECT_EQ(get0.parse_cmd(), "0\n");


    Command incrby { "incrby a 5" };
    EXPECT_EQ(incrby.parse_cmd(), "5\n");
    Command get5 { "get a" };
    EXPECT_EQ(get5.parse_cmd(), "5\n");


    Command decrby { "decrby a 12" };
    EXPECT_EQ(decrby.parse_cmd(), "-7\n");
    Command getn7 { "get a" };
    EXPECT_EQ(getn7.parse_cmd(), "-7\n");


    Command incrbyneg { "incrby a -5" };
    EXPECT_EQ(incrbyneg.parse_cmd(), "-12\n");


    Command decrbyneg { "decrby a -2" };
    EXPECT_EQ(decrbyneg.parse_cmd(), "-10\n");
}

TEST_F(CommandTests, IncrementersInit) {
    Command incr { "incr a" };
    EXPECT_EQ(incr.parse_cmd(), "1\n");
    Command geta { "get a" };
    EXPECT_EQ(geta.parse_cmd(), "1\n");


    Command decr { "decr b" };
    EXPECT_EQ(decr.parse_cmd(), "-1\n");
    Command getb { "get b" };
    EXPECT_EQ(getb.parse_cmd(), "-1\n");


    Command incrby { "incrby c 5" };
    EXPECT_EQ(incrby.parse_cmd(), "5\n");
    Command getc { "get c" };
    EXPECT_EQ(getc.parse_cmd(), "5\n");


    Command decrby { "decrby d 12" };
    EXPECT_EQ(decrby.parse_cmd(), "-12\n");
    Command getd { "get d" };
    EXPECT_EQ(getd.parse_cmd(), "-12\n");
}

TEST_F(CommandTests, ListsPushPop) {
    Command lpush { "lpush a 1 2 3 4" };
    EXPECT_EQ(lpush.parse_cmd(), "4\n");
    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "[4 3 2 1]\n");

    Command llen { "llen a" };
    EXPECT_EQ(llen.parse_cmd(), "4\n");

    Command rpush { "rpush a 0 -1 -2" };
    EXPECT_EQ(rpush.parse_cmd(), "7\n");
    Command get2 { "get a" };
    EXPECT_EQ(get2.parse_cmd(), "[4 3 2 1 0 -1 -2]\n");

    Command llen2 { "llen a" };
    EXPECT_EQ(llen2.parse_cmd(), "7\n");

    Command lpop { "lpop a" };
    EXPECT_EQ(lpop.parse_cmd(), "[4]\n");
    Command get3 { "get a" };
    EXPECT_EQ(get3.parse_cmd(), "[3 2 1 0 -1 -2]\n");

    Command llen3 { "llen a" };
    EXPECT_EQ(llen3.parse_cmd(), "6\n");

    Command lpop2 { "lpop a 2" };
    EXPECT_EQ(lpop2.parse_cmd(), "[3 2]\n");
    Command get4 { "get a" };
    EXPECT_EQ(get4.parse_cmd(), "[1 0 -1 -2]\n");

    Command llen5 { "llen a" };
    EXPECT_EQ(llen5.parse_cmd(), "4\n");

    Command rpop { "rpop a" };
    EXPECT_EQ(rpop.parse_cmd(), "[-2]\n");
    Command get5 { "get a" };
    EXPECT_EQ(get5.parse_cmd(), "[1 0 -1]\n");

    Command llen6 { "llen a" };
    EXPECT_EQ(llen6.parse_cmd(), "3\n");

    Command rpop2 { "rpop a 2" };
    EXPECT_EQ(rpop2.parse_cmd(), "[-1 0]\n");
    Command get6 { "get a" };
    EXPECT_EQ(get6.parse_cmd(), "[1]\n");

    Command llen7 { "llen a" };
    EXPECT_EQ(llen7.parse_cmd(), "1\n");
}


TEST_F(CommandTests, LRange) {
    Command none { "lrange a" };
    EXPECT_EQ(none.parse_cmd(), "(NIL)\n");
    
    Command rpush { "rpush a 1 2 3 4" };
    EXPECT_EQ(rpush.parse_cmd(), "4\n");
    
    Command all { "lrange a" };
    EXPECT_EQ(all.parse_cmd(), "[1 2 3 4]\n");

    Command all2 { "lrange a 0" };
    EXPECT_EQ(all2.parse_cmd(), "[1 2 3 4]\n");

    Command all3 { "lrange a 0 3" };
    EXPECT_EQ(all3.parse_cmd(), "[1 2 3 4]\n");

    Command some { "lrange a 1 3" };
    EXPECT_EQ(some.parse_cmd(), "[2 3 4]\n");

    Command some2 { "lrange a 0 2" };
    EXPECT_EQ(some2.parse_cmd(), "[1 2 3]\n");

    Command middle { "lrange a 1 2" };
    EXPECT_EQ(middle.parse_cmd(), "[2 3]\n");

    Command head { "lrange a 0 0" };
    EXPECT_EQ(head.parse_cmd(), "[1]\n");

    Command tail { "lrange a 3 3" };
    EXPECT_EQ(tail.parse_cmd(), "[4]\n");

    Command out_of_bounds { "lrange a 0 10" };
    EXPECT_EQ(out_of_bounds.parse_cmd(), "[1 2 3 4]\n");

    Command out_of_bounds2 { "lrange a 10 10" };
    EXPECT_EQ(out_of_bounds2.parse_cmd(), "[]\n");

    Command lim { "lrange a -1 -1" };
    EXPECT_EQ(lim.parse_cmd(), "[4]\n");

    Command lim2 { "lrange a 0 -1" };
    EXPECT_EQ(lim2.parse_cmd(), "[1 2 3 4]\n");

    Command lim3 { "lrange a -1 10" };
    EXPECT_EQ(lim3.parse_cmd(), "[4]\n");

    Command negative1 { "lrange a -10 -10" };
    EXPECT_EQ(negative1.parse_cmd(), "ERROR\n");

    Command negative2 { "lrange a 10 -10" };
    EXPECT_EQ(negative2.parse_cmd(), "ERROR\n");

    Command negative3 { "lrange a -10 -10" };
    EXPECT_EQ(negative3.parse_cmd(), "ERROR\n");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "1\n");
}

TEST_F(CommandTests, LRangeEmpty) {
    Command rpush { "rpush a 1 2 3 4" };
    EXPECT_EQ(rpush.parse_cmd(), "4\n");

    Command lpop { "lpop a 10" };
    EXPECT_EQ(lpop.parse_cmd(), "[1 2 3 4]\n");

    Command llen { "llen a" };
    EXPECT_EQ(llen.parse_cmd(), "0\n");

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "[]\n");

    Command empty { "lrange a" };
    EXPECT_EQ(empty.parse_cmd(), "[]\n");

    Command lpop2 { "lpop a" };
    EXPECT_EQ(lpop2.parse_cmd(), "[]\n");
    EXPECT_EQ(get.parse_cmd(), "[]\n");


    Command rpop { "rpop a" };
    EXPECT_EQ(rpop.parse_cmd(), "[]\n");
    EXPECT_EQ(get.parse_cmd(), "[]\n");


    Command rpop2 { "rpop a 10" };
    EXPECT_EQ(rpop2.parse_cmd(), "[]\n");
    EXPECT_EQ(get.parse_cmd(), "[]\n");


    Command rpush2 { "rpush a 1 2 3 4" };
    EXPECT_EQ(rpush2.parse_cmd(), "4\n");
    EXPECT_EQ(get.parse_cmd(), "[1 2 3 4]\n");
    EXPECT_EQ(lpop.parse_cmd(), "[1 2 3 4]\n");
    EXPECT_EQ(get.parse_cmd(), "[]\n");

    Command lpush { "lpush a 1 2 3 4" };
    EXPECT_EQ(lpush.parse_cmd(), "4\n");
    EXPECT_EQ(get.parse_cmd(), "[4 3 2 1]\n");
    EXPECT_EQ(lpop.parse_cmd(), "[4 3 2 1]\n");
    EXPECT_EQ(get.parse_cmd(), "[]\n");
}

TEST_F(CommandTests, NotList) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS\n");

    Command lpush { "lpush a 1" };
    EXPECT_EQ(lpush.parse_cmd(), "NOT A LIST\n");
    
    Command rpush { "rpush a 1" };
    EXPECT_EQ(rpush.parse_cmd(), "NOT A LIST\n");

    Command lpop { "lpop a" };
    EXPECT_EQ(lpop.parse_cmd(), "NOT A LIST\n");

    Command rpop { "rpop a" };
    EXPECT_EQ(rpop.parse_cmd(), "NOT A LIST\n");

    Command llen { "llen a" };
    EXPECT_EQ(llen.parse_cmd(), "NOT A LIST\n");

    Command lrange { "lrange a" };
    EXPECT_EQ(lrange.parse_cmd(), "NOT A LIST\n");
}


TEST_F(CommandTests, SetList) {
    Command lpush { "lpush a 1 2 3" };
    EXPECT_EQ(lpush.parse_cmd(), "3\n");

    Command set { "set a str" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS\n");
    
    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "str\n");

    Command lpop { "lpop a" };
    EXPECT_EQ(lpop.parse_cmd(), "NOT A LIST\n");
}
