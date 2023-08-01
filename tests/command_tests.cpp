#include "gtest/gtest.h"

#include "command.hpp"
#include "globals.hpp"
#include "unix_times.hpp"

bool monitoring = false;
bool stop = false;
LRUCache cache {};
int secs_offset = 0;

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
TEST_F(CommandTests, ExtractName) {
    EXPECT_EQ(cmd::extract_name("1 2 3"), "1");
    EXPECT_EQ(cmd::extract_name(" 1  2 3"), "1");
    EXPECT_EQ(cmd::extract_name("1 2"), "1");
    EXPECT_EQ(cmd::extract_name("1"), "1");
    EXPECT_EQ(cmd::extract_name(""), "");
}
TEST_F(CommandTests, ExtractKey) {
    EXPECT_EQ(cmd::extract_key("1 2 3"), "2");
    EXPECT_EQ(cmd::extract_key(" 1  2 3"), "2");
    EXPECT_EQ(cmd::extract_key("1 2"), "2");
    EXPECT_EQ(cmd::extract_key("1"), "");
    EXPECT_EQ(cmd::extract_key(""), "");
}
TEST_F(CommandTests, Sets) {
    EXPECT_EQ(cmd::addAll("dbsize"), true);
    EXPECT_EQ(cmd::addAll("exists"), true);
    EXPECT_EQ(cmd::addAll("get"), false);

    EXPECT_EQ(cmd::concatAll("keys"), true);
    EXPECT_EQ(cmd::concatAll("get"), false);

    EXPECT_EQ(cmd::askAll("flushall"), true);
    EXPECT_EQ(cmd::askAll("shutdown"), true);
    EXPECT_EQ(cmd::askAll("get"), false);

    EXPECT_EQ(cmd::nodeCmds("create"), cmd::NodeCMDType::Create);
    EXPECT_EQ(cmd::nodeCmds("kill"), cmd::NodeCMDType::Kill);
    EXPECT_EQ(cmd::nodeCmds("nodes"), cmd::NodeCMDType::Nodes);
    EXPECT_EQ(cmd::nodeCmds("get"), cmd::NodeCMDType::Not);
}

TEST_F(CommandTests, EmptyCommand) {
    Command cmd { "" };
    EXPECT_EQ(cmd.parse_cmd(), "No Command Entered");
}

TEST_F(CommandTests, WhitespaceCommand) {
    Command cmd { " " };
    EXPECT_EQ(cmd.parse_cmd(), "No Command Entered");
}

TEST_F(CommandTests, UnknownCommand) {
    Command cmd { "notacommand 1 2 3" };
    EXPECT_EQ(cmd.parse_cmd(), "Invalid Command");
}

TEST_F(CommandTests, Echo) {
    Command cmd { " echo   1   23   45 6 7  8   9   " };
    EXPECT_EQ(cmd.parse_cmd(), "echo 1 23 45 6 7 8 9");
}


TEST_F(CommandTests, ping) {
    Command cmd { "ping" };
    EXPECT_EQ(cmd.parse_cmd(), "PONG");
}

TEST_F(CommandTests, Monitor) {
    Command cmd_admin { "monitor" };
    EXPECT_EQ(cmd_admin.parse_cmd(), "ACTIVE");
    EXPECT_EQ(monitoring, true);

    EXPECT_EQ(cmd_admin.parse_cmd(), "INACTIVE");
    EXPECT_EQ(monitoring, false);
}

TEST_F(CommandTests, Shutdown) {
    Command cmd_admin { "shutdown" };
    EXPECT_EQ(cmd_admin.parse_cmd(), "Stopping server...");
    EXPECT_EQ(stop, true);
}


TEST_F(CommandTests, Keys) {
    Command keys_none { "keys" };
    EXPECT_EQ(keys_none.parse_cmd(), "");

    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS");

    Command set_C { "set c 3" };
    EXPECT_EQ(set_C.parse_cmd(), "SUCCESS");

    Command keys_1 { "keys" };
    EXPECT_EQ(keys_1.parse_cmd(), "a b c");

    Command del_b { "del b" };
    EXPECT_EQ(del_b.parse_cmd(), "1");

    Command keys_2 { "keys" };
    EXPECT_EQ(keys_2.parse_cmd(), "a c");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "2");
}

TEST_F(CommandTests, Benchmark) {
    Command benchmark { "benchmark 10" };
    std::string run_time = benchmark.parse_cmd();

    EXPECT_TRUE(run_time == "0 ms" || "1 ms");
}

TEST_F(CommandTests, GetSet) {
    Command get_dne { "get a" };
    EXPECT_EQ(get_dne.parse_cmd(), "(NIL)");

    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS");

    Command get_b { "get b" };
    EXPECT_EQ(get_b.parse_cmd(), "2");

    Command overwrite_a { "set a 3"};
    EXPECT_EQ(overwrite_a.parse_cmd(), "SUCCESS");

    Command get_new_a { "get a" };
    EXPECT_EQ(get_new_a.parse_cmd(), "3");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "2");
}


TEST_F(CommandTests, Rename) {
    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command rename { "rename a b" };
    EXPECT_EQ(rename.parse_cmd(), "SUCCESS");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "(NIL)");

    Command get_b { "get b" };
    EXPECT_EQ(get_b.parse_cmd(), "1");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1");
}

TEST_F(CommandTests, RenameOverwrite) {
    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS");

    Command rename { "rename a b" };
    EXPECT_EQ(rename.parse_cmd(), "SUCCESS");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "(NIL)");

    Command get_b { "get b" };
    EXPECT_EQ(get_b.parse_cmd(), "1");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1");
}

TEST_F(CommandTests, Del) {
    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS");

    Command set_c { "set c 3" };
    EXPECT_EQ(set_c.parse_cmd(), "SUCCESS");

    Command remove_none { "del "};
    EXPECT_EQ(remove_none.parse_cmd(), "0");

    Command remove_some { "del a b"};
    EXPECT_EQ(remove_some.parse_cmd(), "2");

    Command remove_dne { "del a b"};
    EXPECT_EQ(remove_dne.parse_cmd(), "0");

    Command remove_randoms { "del c d a e"};
    EXPECT_EQ(remove_randoms.parse_cmd(), "1");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "0");
}

TEST_F(CommandTests, Exists) {
    Command exists_empty { "exists a b c" };
    EXPECT_EQ(exists_empty.parse_cmd(), "0");

    Command set_a { "set a 1" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command set_b { "set b 2" };
    EXPECT_EQ(set_b.parse_cmd(), "SUCCESS");

    Command set_c { "set c 3" };
    EXPECT_EQ(set_c.parse_cmd(), "SUCCESS");

    Command exists_none { "exists" };
    EXPECT_EQ(exists_none.parse_cmd(), "0");

    Command exists_all { "exists a b c" };
    EXPECT_EQ(exists_all.parse_cmd(), "3");

    Command exists_some { "exists a c" };
    EXPECT_EQ(exists_some.parse_cmd(), "2");

    Command exists_not_included { "exists a z y 7" };
    EXPECT_EQ(exists_not_included.parse_cmd(), "1");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "3");
}


TEST_F(CommandTests, Expire) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS");

    Command expire { "expire a 50" };
    std::string res = expire.parse_cmd();
    std::string time = std::to_string(time_secs() + 50) + "";
    //in case it is at the end of the second
    std::string close = std::to_string(time_secs() + 51) + "";
    EXPECT_TRUE(res == time || res == close);

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1");

    //offset the time_secs() function;
    secs_offset = 100;

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1");

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "(NIL)");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "0");

    Command dbsize_rm { "dbsize" };
    EXPECT_EQ(dbsize_rm.parse_cmd(), "0");
}

TEST_F(CommandTests, ExpireAt) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS");

    Command expire { "expireat a " + std::to_string(time_secs() + 50)};
    EXPECT_EQ(expire.parse_cmd(), "SUCCESS");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "1");

    //offset the time_secs() function;
    secs_offset = 100;

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "(NIL)");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "0");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "0");
}


TEST_F(CommandTests, Persist) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS");

    Command persist_fail { "persist a" };
    EXPECT_EQ(persist_fail.parse_cmd(), "FAILURE");

    Command expire { "expire a 50" };
    expire.parse_cmd();

    Command persist { "persist a" };
    EXPECT_EQ(persist.parse_cmd(), "SUCCESS");


    //offset the time_secs() function;
    secs_offset = 100;

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "1");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "1");
}



TEST_F(CommandTests, IncrementersNotNum) {
    Command set_a { "set a notanum" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");

    Command incr_fail { "incr a" };
    EXPECT_EQ(incr_fail.parse_cmd(), "NOT AN INT");

    Command incrby_fail { "incrby a 2" };
    EXPECT_EQ(incrby_fail.parse_cmd(), "NOT AN INT");

    Command decr_fail { "decr a" };
    EXPECT_EQ(decr_fail.parse_cmd(), "NOT AN INT");

    Command decrby_fail { "decrby a 2" };
    EXPECT_EQ(decrby_fail.parse_cmd(), "NOT AN INT");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "1");
}

TEST_F(CommandTests, Incrementers) {
    Command set_a { "set a 0" };
    EXPECT_EQ(set_a.parse_cmd(), "SUCCESS");


    Command incr { "incr a" };
    EXPECT_EQ(incr.parse_cmd(), "1");
    Command get1 { "get a" };
    EXPECT_EQ(get1.parse_cmd(), "1");


    Command decr { "decr a" };
    EXPECT_EQ(decr.parse_cmd(), "0");
    Command get0 { "get a" };
    EXPECT_EQ(get0.parse_cmd(), "0");


    Command incrby { "incrby a 5" };
    EXPECT_EQ(incrby.parse_cmd(), "5");
    Command get5 { "get a" };
    EXPECT_EQ(get5.parse_cmd(), "5");


    Command decrby { "decrby a 12" };
    EXPECT_EQ(decrby.parse_cmd(), "-7");
    Command getn7 { "get a" };
    EXPECT_EQ(getn7.parse_cmd(), "-7");


    Command incrbyneg { "incrby a -5" };
    EXPECT_EQ(incrbyneg.parse_cmd(), "-12");


    Command decrbyneg { "decrby a -2" };
    EXPECT_EQ(decrbyneg.parse_cmd(), "-10");
}

TEST_F(CommandTests, IncrementersInit) {
    Command incr { "incr a" };
    EXPECT_EQ(incr.parse_cmd(), "1");
    Command geta { "get a" };
    EXPECT_EQ(geta.parse_cmd(), "1");


    Command decr { "decr b" };
    EXPECT_EQ(decr.parse_cmd(), "-1");
    Command getb { "get b" };
    EXPECT_EQ(getb.parse_cmd(), "-1");


    Command incrby { "incrby c 5" };
    EXPECT_EQ(incrby.parse_cmd(), "5");
    Command getc { "get c" };
    EXPECT_EQ(getc.parse_cmd(), "5");


    Command decrby { "decrby d 12" };
    EXPECT_EQ(decrby.parse_cmd(), "-12");
    Command getd { "get d" };
    EXPECT_EQ(getd.parse_cmd(), "-12");
}

TEST_F(CommandTests, ListsPushPop) {
    Command lpush { "lpush a 1 2 3 4" };
    EXPECT_EQ(lpush.parse_cmd(), "4");
    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "4 3 2 1");

    Command llen { "llen a" };
    EXPECT_EQ(llen.parse_cmd(), "4");

    Command rpush { "rpush a 0 -1 -2" };
    EXPECT_EQ(rpush.parse_cmd(), "7");
    Command get2 { "get a" };
    EXPECT_EQ(get2.parse_cmd(), "4 3 2 1 0 -1 -2");

    Command llen2 { "llen a" };
    EXPECT_EQ(llen2.parse_cmd(), "7");

    Command lpop { "lpop a" };
    EXPECT_EQ(lpop.parse_cmd(), "4");
    Command get3 { "get a" };
    EXPECT_EQ(get3.parse_cmd(), "3 2 1 0 -1 -2");

    Command llen3 { "llen a" };
    EXPECT_EQ(llen3.parse_cmd(), "6");

    Command lpop2 { "lpop a 2" };
    EXPECT_EQ(lpop2.parse_cmd(), "3 2");
    Command get4 { "get a" };
    EXPECT_EQ(get4.parse_cmd(), "1 0 -1 -2");

    Command llen5 { "llen a" };
    EXPECT_EQ(llen5.parse_cmd(), "4");

    Command rpop { "rpop a" };
    EXPECT_EQ(rpop.parse_cmd(), "-2");
    Command get5 { "get a" };
    EXPECT_EQ(get5.parse_cmd(), "1 0 -1");

    Command llen6 { "llen a" };
    EXPECT_EQ(llen6.parse_cmd(), "3");

    Command rpop2 { "rpop a 2" };
    EXPECT_EQ(rpop2.parse_cmd(), "-1 0");
    Command get6 { "get a" };
    EXPECT_EQ(get6.parse_cmd(), "1");

    Command llen7 { "llen a" };
    EXPECT_EQ(llen7.parse_cmd(), "1");
}


TEST_F(CommandTests, LRange) {
    Command none { "lrange a" };
    EXPECT_EQ(none.parse_cmd(), "(NIL)");
    
    Command rpush { "rpush a 1 2 3 4" };
    EXPECT_EQ(rpush.parse_cmd(), "4");
    
    Command all { "lrange a" };
    EXPECT_EQ(all.parse_cmd(), "1 2 3 4");

    Command all2 { "lrange a 0" };
    EXPECT_EQ(all2.parse_cmd(), "1 2 3 4");

    Command all3 { "lrange a 0 3" };
    EXPECT_EQ(all3.parse_cmd(), "1 2 3 4");

    Command some { "lrange a 1 3" };
    EXPECT_EQ(some.parse_cmd(), "2 3 4");

    Command some2 { "lrange a 0 2" };
    EXPECT_EQ(some2.parse_cmd(), "1 2 3");

    Command middle { "lrange a 1 2" };
    EXPECT_EQ(middle.parse_cmd(), "2 3");

    Command head { "lrange a 0 0" };
    EXPECT_EQ(head.parse_cmd(), "1");

    Command tail { "lrange a 3 3" };
    EXPECT_EQ(tail.parse_cmd(), "4");

    Command out_of_bounds { "lrange a 0 10" };
    EXPECT_EQ(out_of_bounds.parse_cmd(), "1 2 3 4");

    Command out_of_bounds2 { "lrange a 10 10" };
    EXPECT_EQ(out_of_bounds2.parse_cmd(), "");

    Command lim { "lrange a -1 -1" };
    EXPECT_EQ(lim.parse_cmd(), "4");

    Command lim2 { "lrange a 0 -1" };
    EXPECT_EQ(lim2.parse_cmd(), "1 2 3 4");

    Command lim3 { "lrange a -1 10" };
    EXPECT_EQ(lim3.parse_cmd(), "4");

    Command negative1 { "lrange a -10 -10" };
    EXPECT_EQ(negative1.parse_cmd(), "ERROR");

    Command negative2 { "lrange a 10 -10" };
    EXPECT_EQ(negative2.parse_cmd(), "ERROR");

    Command negative3 { "lrange a -10 -10" };
    EXPECT_EQ(negative3.parse_cmd(), "ERROR");

    Command del { "del a" };
    EXPECT_EQ(del.parse_cmd(), "1");
}

TEST_F(CommandTests, LRangeEmpty) {
    Command rpush { "rpush a 1 2 3 4" };
    EXPECT_EQ(rpush.parse_cmd(), "4");

    Command lpop { "lpop a 10" };
    EXPECT_EQ(lpop.parse_cmd(), "1 2 3 4");

    Command llen { "llen a" };
    EXPECT_EQ(llen.parse_cmd(), "0");

    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "");

    Command empty { "lrange a" };
    EXPECT_EQ(empty.parse_cmd(), "");

    Command lpop2 { "lpop a" };
    EXPECT_EQ(lpop2.parse_cmd(), "");
    EXPECT_EQ(get.parse_cmd(), "");


    Command rpop { "rpop a" };
    EXPECT_EQ(rpop.parse_cmd(), "");
    EXPECT_EQ(get.parse_cmd(), "");


    Command rpop2 { "rpop a 10" };
    EXPECT_EQ(rpop2.parse_cmd(), "");
    EXPECT_EQ(get.parse_cmd(), "");


    Command rpush2 { "rpush a 1 2 3 4" };
    EXPECT_EQ(rpush2.parse_cmd(), "4");
    EXPECT_EQ(get.parse_cmd(), "1 2 3 4");
    EXPECT_EQ(lpop.parse_cmd(), "1 2 3 4");
    EXPECT_EQ(get.parse_cmd(), "");

    Command lpush { "lpush a 1 2 3 4" };
    EXPECT_EQ(lpush.parse_cmd(), "4");
    EXPECT_EQ(get.parse_cmd(), "4 3 2 1");
    EXPECT_EQ(lpop.parse_cmd(), "4 3 2 1");
    EXPECT_EQ(get.parse_cmd(), "");
}

TEST_F(CommandTests, NotList) {
    Command set { "set a 1" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS");

    Command lpush { "lpush a 1" };
    EXPECT_EQ(lpush.parse_cmd(), "NOT A LIST");
    
    Command rpush { "rpush a 1" };
    EXPECT_EQ(rpush.parse_cmd(), "NOT A LIST");

    Command lpop { "lpop a" };
    EXPECT_EQ(lpop.parse_cmd(), "NOT A LIST");

    Command rpop { "rpop a" };
    EXPECT_EQ(rpop.parse_cmd(), "NOT A LIST");

    Command llen { "llen a" };
    EXPECT_EQ(llen.parse_cmd(), "NOT A LIST");

    Command lrange { "lrange a" };
    EXPECT_EQ(lrange.parse_cmd(), "NOT A LIST");
}


TEST_F(CommandTests, SetList) {
    Command lpush { "lpush a 1 2 3" };
    EXPECT_EQ(lpush.parse_cmd(), "3");

    Command set { "set a str" };
    EXPECT_EQ(set.parse_cmd(), "SUCCESS");
    
    Command get { "get a" };
    EXPECT_EQ(get.parse_cmd(), "str");

    Command lpop { "lpop a" };
    EXPECT_EQ(lpop.parse_cmd(), "NOT A LIST");
}

TEST_F(CommandTests, Types) {
    Command str { "set a hello" };
    EXPECT_EQ(str.parse_cmd(), "SUCCESS");
    Command str_type { "type a" };
    EXPECT_EQ(str_type.parse_cmd(), "string");
    
    Command integer { "set b 1" };
    EXPECT_EQ(integer.parse_cmd(), "SUCCESS");
    Command integer_type { "type b" };
    EXPECT_EQ(integer_type.parse_cmd(), "int");

    Command list { "rpush c 1 2 3 4" };
    EXPECT_EQ(list.parse_cmd(), "4");
    Command list_type { "type c" };
    EXPECT_EQ(list_type.parse_cmd(), "list");
}

TEST_F(CommandTests, flushall) {
    Command str { "set a hello" };
    EXPECT_EQ(str.parse_cmd(), "SUCCESS");
        
    Command integer { "set b 1" };
    EXPECT_EQ(integer.parse_cmd(), "SUCCESS");

    Command list { "rpush c 1 2 3 4" };
    EXPECT_EQ(list.parse_cmd(), "4");


    Command flushall { "flushall" };
    EXPECT_EQ(flushall.parse_cmd(), "Cache cleared");

    Command dbsize { "dbsize" };
    EXPECT_EQ(dbsize.parse_cmd(), "0");

    Command get_a { "get a" };
    EXPECT_EQ(get_a.parse_cmd(), "(NIL)");

    Command get_b { "get b" };
    EXPECT_EQ(get_b.parse_cmd(), "(NIL)");

    Command get_c { "get c" };
    EXPECT_EQ(get_c.parse_cmd(), "(NIL)");

    Command keys { "keys" };
    EXPECT_EQ(keys.parse_cmd(), "");
}
