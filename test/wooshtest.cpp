#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE WooshTest
#include <boost/test/unit_test.hpp>
#include <unordered_map>
#include <string>
#include <utility>
#include "../include/woosh.h"
#include "../include/history.h"

using std::string;
using std::unordered_map;

//////////////////////////
/*
  Test changing directories
  
*/////////////////////////

struct changingDirsFixture {
  History *hist = hist->getInstance();
};
//string builtInCd(string token, History* history);

BOOST_FIXTURE_TEST_SUITE(builtInCdTests, changingDirsFixture)

BOOST_AUTO_TEST_CASE(cd_home_is_correct) {
  BOOST_TEST(builtInCd("~",hist)=="/home/rymwoo");
}

BOOST_AUTO_TEST_CASE(going_up_and_down_directories) {
  builtInCd("/home/rymwoo/Desktop/Programming/cpp",hist);
  BOOST_TEST(builtInCd("../../Programming/python",hist)=="/home/rymwoo/Desktop/Programming/python");
}

BOOST_AUTO_TEST_CASE(returning_to_previous_dir) {
  builtInCd("/home/rymwoo/Desktop/Programming/cpp",hist);
  builtInCd("../python",hist);
  BOOST_TEST(builtInCd("-",hist)=="/home/rymwoo/Desktop/Programming/cpp");
}

BOOST_AUTO_TEST_CASE(goto_subfolder) {
  builtInCd("/home/rymwoo/Desktop/Programming",hist);
  BOOST_TEST(builtInCd("cpp",hist)=="/home/rymwoo/Desktop/Programming/cpp");
}

BOOST_AUTO_TEST_SUITE_END()


//////////////////////////
/*
  Test replacing aliases
  
*/////////////////////////

struct replaceAliasFixture {
  replaceAliasFixture() {
//    unordered_map<string,string> aliases;
    aliases["a"]="bbb";
    aliases["b"]="ccc";
  }
  string input;
  unordered_map<string,string> aliases;
};
//void replaceAliases(string &input, unordered_map<string,string> &aliases);

BOOST_FIXTURE_TEST_SUITE(replaceAliasesTests, replaceAliasFixture)

BOOST_AUTO_TEST_CASE(alias_start_of_input){
  input="a sdfg";
  replaceAliases(input,aliases);
  BOOST_TEST(input == "bbb sdfg");
}

BOOST_AUTO_TEST_CASE(alias_end_of_input){
  input="squi b";
  replaceAliases(input,aliases);
  BOOST_TEST(input=="squi ccc");
}

BOOST_AUTO_TEST_CASE(entire_input){
  input="a";
  replaceAliases(input,aliases);
  BOOST_TEST(input=="bbb");
}

BOOST_AUTO_TEST_CASE(multiple_aliases){
  input="a b a cus";
  replaceAliases(input,aliases);
  BOOST_TEST(input=="bbb ccc bbb cus");
}

BOOST_AUTO_TEST_CASE(no_alias_replacement_midtoken){
  input="caws";
  replaceAliases(input,aliases);
  BOOST_TEST(input=="caws");
}

BOOST_AUTO_TEST_CASE(no_matching_alias){
  input="kiwis";
  replaceAliases(input,aliases);
  BOOST_TEST(input=="kiwis");
}

BOOST_AUTO_TEST_CASE(alias_ignored_inside_quote){
  input="is this \"not a bird\"?";
    replaceAliases(input,aliases);
  BOOST_TEST(input=="is this \"not a bird\"?");
}

BOOST_AUTO_TEST_SUITE_END()

//////////////////////////
/*
  Test history expansion
  
*/////////////////////////

struct historyExpansionFixture {
  historyExpansionFixture() {
    hist->clearHistory();
    hist->push_back("cd ..");
    hist->push_back("ls -a");
    hist->push_back("cd /usr/bin");
    hist->push_back("cd -");
  }
  History *hist = hist->getInstance();
  string input;
};
//void historyExpansion(string &input, History* history);

BOOST_FIXTURE_TEST_SUITE(historyExpansionTests, historyExpansionFixture)

BOOST_AUTO_TEST_CASE(history_expansion_by_number) {
  input = "abc !1 def";
  historyExpansion(input, hist);
  BOOST_TEST(input == "abc cd .. def");
}

BOOST_AUTO_TEST_CASE(history_expansion_of_previous) {
  input = "abc !! def";
  historyExpansion(input, hist);
  BOOST_TEST(input == "abc cd - def");
}

BOOST_AUTO_TEST_CASE(history_expansion_midtoken) {
  input = "abc!!def";
  historyExpansion(input, hist);
  BOOST_TEST(input == "abccd -def");
}

BOOST_AUTO_TEST_CASE(history_expansion_nonexistent) {
  input = "abc !95 !-5 def";
  historyExpansion(input, hist);
  BOOST_TEST(input == "abc   def");
}

BOOST_AUTO_TEST_CASE(history_expansion_ignored_inside_quotes) {
  input = "abc \"as !95 sd\" def";
  historyExpansion(input, hist);
  BOOST_TEST(input == "abc \"as !95 sd\" def");
}

BOOST_AUTO_TEST_CASE(history_expansion_from_backwards) {
  input = "abc !-2 def";
  historyExpansion(input, hist);
  BOOST_TEST(input == "abc cd /usr/bin def");
}

BOOST_AUTO_TEST_CASE(history_expansion_replace_entirety) {
  input = "!-3";
  historyExpansion(input, hist);
  BOOST_TEST(input == "ls -a");
}

BOOST_AUTO_TEST_SUITE_END()