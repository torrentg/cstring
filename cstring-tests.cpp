#define CATCH_CONFIG_MAIN

#include <sstream>
#include <cstring>
#include "catch.hpp"
#include "cstring.hpp"

using namespace std;
using namespace gto;

// g++ -g --coverage -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion test.cpp -I../deps/
// valgrind --tool=memcheck --leak-check=yes ./a.out
// lcov -d . --no-external -o coverage.info -c
// genhtml -o coverage coverage.info
// firefox coverage/index.html
TEST_CASE("cstring") {

  SECTION("sizeof") {
    CHECK(sizeof(cstring) == sizeof(char *));
  }

  SECTION("constructor (empty string)") {
    cstring empty1;
    CHECK(empty1.size() == 0);
    CHECK(empty1.length() == 0);
    CHECK(std::strlen(empty1.data()) == 0);
    {
      cstring empty2(nullptr);
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2(nullptr, 10);
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2("");
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2("abc", 0);
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2(empty1);
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2 = empty1;
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2 = cstring();
      CHECK(empty1.data() == empty2.data());
    }
    {
      cstring empty2(cstring{});
      CHECK(empty1.data() == empty2.data());
    }
  }

  SECTION("constructor (generic)") {
    {
      cstring str("abc");
      CHECK(str.size() == 3);
      CHECK(str.length() == 3);
      CHECK(string("abc") == str.data());
    }
    {
      cstring str("abc", 2);
      CHECK(str.size() == 2);
      CHECK(string("ab") == str.data());
    }
    {
      cstring str1("abc");
      cstring str2(str1);
      CHECK(str1.data() == str2.data());
    }
    {
      cstring str1("abc");
      cstring str2 = str1;
      CHECK(str1.data() == str2.data());
    }
    {
      cstring str1("abc");
      const char *ptr = str1.data();
      cstring str2(std::move(str1));
      CHECK(str2.data() == ptr);
    }
  }

  SECTION("assignment") {
    {
      cstring str1("abc");
      cstring str2("xyz");
      CHECK(str1.data() != str2.data());
      str1 = str2;
      CHECK(str1.data() == str2.data());
      CHECK(str1.data() == string("xyz"));
    }
    {
      cstring str1("abc");
      cstring str2;
      CHECK(str1.data() != str2.data());
      str1 = str2;
      CHECK(str1.data() == str2.data());
      CHECK(str1.data() == string(""));
    }
  }

  SECTION("move") {
    cstring str1("abc");
    cstring str2("xyz");
    CHECK(str1.data() != str2.data());
    str1 = std::move(str2);
    CHECK(str1.data() != str2.data());
    CHECK(str1.data() == string("xyz"));
    CHECK(str2.data() == string("abc"));
  }

  SECTION("empty") {
    CHECK(!cstring("abc").empty());
    CHECK(cstring("").empty());
  }

  SECTION("get char") {
    cstring str1("abc");
    CHECK(str1[0] == 'a');
    CHECK(str1.at(0) == 'a');
    CHECK(str1[1] == 'b');
    CHECK(str1.at(1) == 'b');
    CHECK(str1[2] == 'c');
    CHECK(str1.at(2) == 'c');
    CHECK_THROWS(str1.at(3));
    CHECK(str1.front() == 'a');
    CHECK(str1.back() == 'c');
  }

  SECTION("data") {
    cstring str("abc");
    CHECK(std::strcmp(str.data(), "abc") == 0);
    CHECK(std::strcmp(str.c_str(), "abc") == 0);
  }

  SECTION("iterator") {
    cstring str("abc");
    auto it = str.cbegin();
    CHECK(*it == 'a');
    it++;
    CHECK(*it == 'b');
    ++it;
    CHECK(*it == 'c');
    it++;
    CHECK(it == str.cend());
  }

  SECTION("reverse iterator") {
    cstring str("abc");
    auto it = str.crbegin();
    CHECK(*it == 'c');
    it++;
    CHECK(*it == 'b');
    ++it;
    CHECK(*it == 'a');
    it++;
    CHECK(it == str.crend());
  }

  SECTION("swap") {
    cstring str1("abc");
    cstring str2("xyz");
    str1.swap(str2);
    CHECK(str1.data() == std::string("xyz"));
    CHECK(str2.data() == std::string("abc"));
  }

  SECTION("substr") {
    cstring str("hello world");
    CHECK(str.substr() == std::string("hello world"));
    CHECK(str.substr(0) == std::string("hello world"));
    CHECK(str.substr(6) == std::string("world"));
    CHECK(str.substr(0, 999) == std::string("hello world"));
    CHECK(str.substr(6, 999) == std::string("world"));
    CHECK(str.substr(11) == std::string(""));
    CHECK_THROWS(str.substr(999));
    CHECK(str.substr(0, 4) == std::string("hell"));
    CHECK(str.substr(6, 5) == std::string("world"));
    CHECK(str.substr(6, 999) == std::string("world"));
  }

  SECTION("compare") {
    CHECK(cstring("").compare(cstring("")) == 0);
    CHECK(cstring("").compare(cstring("abc")) < 0);
    CHECK(cstring("abc").compare(cstring("")) > 0);

    CHECK(cstring("abc").compare(cstring("abc")) == 0);
    CHECK(cstring("abc").compare(cstring("aba")) > 0);
    CHECK(cstring("abc").compare(cstring("abx")) < 0);

    CHECK(cstring("hello world").compare(6, 5, cstring("world")) == 0);
    CHECK(cstring("hello world").compare(6, 5, cstring("worla")) > 0);
    CHECK(cstring("hello world").compare(6, 5, cstring("worlx")) < 0);

    CHECK(cstring("hello world").compare(6, 5, cstring("a world"), 2, 5) == 0);
    CHECK(cstring("hello world").compare(6, 5, cstring("a worla"), 2, 5) > 0);
    CHECK(cstring("hello world").compare(6, 5, cstring("a world x"), 2, 7) < 0);

    CHECK(cstring("").compare("") == 0);
    CHECK(cstring("").compare(nullptr) == 0);
    CHECK(cstring("").compare("abc") < 0);
    CHECK(cstring("abc").compare("") > 0);
    CHECK(cstring("abc").compare(nullptr) > 0);

    CHECK(cstring("abc").compare("abc") == 0);
    CHECK(cstring("abc").compare("aba") > 0);
    CHECK(cstring("abc").compare("abx") < 0);

    CHECK(cstring("hello world").compare(6, 5, nullptr) > 0);
    CHECK(cstring("hello world").compare(6, 5, "world") == 0);
    CHECK(cstring("hello world").compare(6, 5, "worla") > 0);
    CHECK(cstring("hello world").compare(6, 5, "worlx") < 0);

    CHECK(cstring("hello world").compare(6, 5, nullptr, 5) > 0);
    CHECK(cstring("hello world").compare(6, 5, "world x", 5) == 0);
    CHECK(cstring("hello world").compare(6, 5, "world x", 4) > 0);
    CHECK(cstring("hello world").compare(6, 5, "world x", 6) < 0);
  }

  SECTION("starts_with") {
    CHECK(cstring("hello world").starts_with(cstring("hell")));
    CHECK(!cstring("hello world").starts_with(cstring("ello")));

    const char *aux = "hello";
    CHECK(cstring("hello world").starts_with(cstring::basic_cstring_view(aux)));
    CHECK(!cstring("hello world").starts_with(cstring::basic_cstring_view(aux, 1)));

    CHECK(cstring("hello world").starts_with(nullptr));
    CHECK(cstring("hello world").starts_with(""));
    CHECK(cstring("hello world").starts_with("hell"));
    CHECK(!cstring("hello world").starts_with("ello"));
  }

  SECTION("ends_with") {
    CHECK(cstring("hello world").ends_with(cstring("world")));
    CHECK(!cstring("hello world").ends_with(cstring("worlds")));

    const char *aux = "world";
    CHECK(cstring("hello world").ends_with(cstring::basic_cstring_view(aux)));
    CHECK(!cstring("hello world").ends_with(cstring::basic_cstring_view(aux, 4)));

    CHECK(cstring("hello world").ends_with(nullptr));
    CHECK(cstring("hello world").ends_with(""));
    CHECK(cstring("hello world").ends_with("world"));
    CHECK(!cstring("hello world").ends_with("worl"));
  }

  SECTION("find") {
    CHECK(cstring("abcdef").find(cstring("")) == 0);
    CHECK(cstring("abcdef").find(cstring("cde")) == 2);
    CHECK(cstring("abcdef").find(cstring("cde"), 0) == 2);
    CHECK(cstring("abcdef").find(cstring("cde"), 1) == 2);
    CHECK(cstring("abcdef").find(cstring("cde"), 2) == 2);
    CHECK(cstring("abcdef").find(cstring("cde"), 3) == cstring::npos);

    CHECK(cstring("abcdef").find(nullptr) == 0);
    CHECK(cstring("abcdef").find("") == 0);
    CHECK(cstring("abcdef").find("cde") == 2);

    CHECK(cstring("abcdef").find(nullptr, 1) == 1);
    CHECK(cstring("abcdef").find("", 2) == 2);
    CHECK(cstring("abcdef").find("cde", 0) == 2);
    CHECK(cstring("abcdef").find("cde", 1) == 2);
    CHECK(cstring("abcdef").find("cde", 2) == 2);
    CHECK(cstring("abcdef").find("cde", 3) == cstring::npos);

    CHECK(cstring("abcdef").find(nullptr, 0, 3) == cstring::npos);
    CHECK(cstring("abcdef").find("", 1, 3) == cstring::npos);
    CHECK(cstring("abcdef").find("cde", 0, 3) == 2);
    CHECK(cstring("abcdef").find("cde", 1, 3) == 2);
    CHECK(cstring("abcdef").find("cde", 2, 3) == 2);
    CHECK(cstring("abcdef").find("cde", 3, 3) == cstring::npos);

    CHECK(cstring("abcdef").find('c') == 2);
    CHECK(cstring("abcdef").find('c', 1) == 2);
    CHECK(cstring("abcdef").find('c', 2) == 2);
    CHECK(cstring("abcdef").find('c', 3) == cstring::npos);
  }

  SECTION("rfind") {
    CHECK(cstring("abcdef").rfind(cstring("cde")) == 2);
    CHECK(cstring("abcdef").rfind(cstring("cde"), 7) == 2);
    CHECK(cstring("abcdef").rfind(cstring("cde"), 6) == 2);
    CHECK(cstring("abcdef").rfind(cstring("cde"), 5) == 2);
    CHECK(cstring("abcdef").rfind(cstring("cde"), 4) == 2); // !
    CHECK(cstring("abcdef").rfind(cstring("cde"), 3) == 2); // !
    CHECK(cstring("abcdef").rfind(cstring("cde"), 2) == 2); // !
    CHECK(cstring("abcdef").rfind(cstring("cde"), 1) == cstring::npos);

    CHECK(cstring("abcdef").rfind(nullptr) == 6);
    CHECK(cstring("abcdef").rfind("") == 6);
    CHECK(cstring("abcdef").rfind("cde") == 2);

    CHECK(cstring("abcdef").rfind(nullptr, 1) == 1);
    CHECK(cstring("abcdef").rfind("", 2) == 2);
    CHECK(cstring("abcdef").rfind("cde", 7) == 2);
    CHECK(cstring("abcdef").rfind("cde", 6) == 2);
    CHECK(cstring("abcdef").rfind("cde", 5) == 2);
    CHECK(cstring("abcdef").rfind("cde", 4) == 2);
    CHECK(cstring("abcdef").rfind("cde", 3) == 2);
    CHECK(cstring("abcdef").rfind("cde", 2) == 2);
    CHECK(cstring("abcdef").rfind("cde", 1) == cstring::npos);

    CHECK(cstring("abcdef").rfind(nullptr, 4, 3) == cstring::npos);
    CHECK(cstring("abcdef").rfind("", 4, 3) == cstring::npos);
    CHECK(cstring("abcdef").rfind("cde", 4, 3) == 2);
    CHECK(cstring("abcdef").rfind("cde", 3, 3) == 2);
    CHECK(cstring("abcdef").rfind("cde", 2, 3) == 2);
    CHECK(cstring("abcdef").rfind("cde", 1, 3) == cstring::npos);

    CHECK(cstring("abcdef").rfind('c') == 2);
    CHECK(cstring("abcdef").rfind('c', 3) == 2);
    CHECK(cstring("abcdef").rfind('c', 2) == 2);
    CHECK(cstring("abcdef").rfind('c', 1) == cstring::npos);
  }

  SECTION("find_first_of") {
    CHECK(cstring("abcdef").find_first_of(cstring("dc")) == 2);
    CHECK(cstring("abcdef").find_first_of(cstring("dc"), 0) == 2);
    CHECK(cstring("abcdef").find_first_of(cstring("dc"), 1) == 2);
    CHECK(cstring("abcdef").find_first_of(cstring("dc"), 2) == 2);
    CHECK(cstring("abcdef").find_first_of(cstring("dc"), 3) == 3);
    CHECK(cstring("abcdef").find_first_of(cstring("dc"), 4) == cstring::npos);

    CHECK(cstring("abcdef").find_first_of(nullptr) == cstring::npos);
    CHECK(cstring("abcdef").find_first_of("") == cstring::npos);
    CHECK(cstring("abcdef").find_first_of("dc") == 2);

    CHECK(cstring("abcdef").find_first_of(nullptr, 0) == cstring::npos);
    CHECK(cstring("abcdef").find_first_of("dc", 0) == 2);
    CHECK(cstring("abcdef").find_first_of("dc", 1) == 2);
    CHECK(cstring("abcdef").find_first_of("dc", 2) == 2);
    CHECK(cstring("abcdef").find_first_of("dc", 3) == 3);
    CHECK(cstring("abcdef").find_first_of("dc", 4) == cstring::npos);

    CHECK(cstring("abcdef").find_first_of(nullptr, 0, 1) == cstring::npos);
    CHECK(cstring("abcdef").find_first_of("dc", 0, 1) == 3);
    CHECK(cstring("abcdef").find_first_of("dc", 1, 1) == 3);
    CHECK(cstring("abcdef").find_first_of("dc", 2, 1) == 3);
    CHECK(cstring("abcdef").find_first_of("dc", 3, 1) == 3);
    CHECK(cstring("abcdef").find_first_of("dc", 4, 1) == cstring::npos);

    CHECK(cstring("abcdef").find_first_of('c') == 2);
    CHECK(cstring("abcdef").find_first_of('c', 0) == 2);
    CHECK(cstring("abcdef").find_first_of('c', 1) == 2);
    CHECK(cstring("abcdef").find_first_of('c', 2) == 2);
    CHECK(cstring("abcdef").find_first_of('c', 3) == cstring::npos);
  }

  SECTION("find_first_not_of") {
    CHECK(cstring("abcdef").find_first_not_of(cstring("abef")) == 2);
    CHECK(cstring("abcdef").find_first_not_of(cstring("abef"), 0) == 2);
    CHECK(cstring("abcdef").find_first_not_of(cstring("abef"), 1) == 2);
    CHECK(cstring("abcdef").find_first_not_of(cstring("abef"), 2) == 2);
    CHECK(cstring("abcdef").find_first_not_of(cstring("abef"), 3) == 3);
    CHECK(cstring("abcdef").find_first_not_of(cstring("abef"), 4) == cstring::npos);

    CHECK(cstring("abcdef").find_first_not_of(nullptr) == 0);
    CHECK(cstring("abcdef").find_first_not_of("") == 0);
    CHECK(cstring("abcdef").find_first_not_of("abef") == 2);

    CHECK(cstring("abcdef").find_first_not_of(nullptr, 1) == 1);
    CHECK(cstring("abcdef").find_first_not_of("abef", 0) == 2);
    CHECK(cstring("abcdef").find_first_not_of("abef", 1) == 2);
    CHECK(cstring("abcdef").find_first_not_of("abef", 2) == 2);
    CHECK(cstring("abcdef").find_first_not_of("abef", 3) == 3);
    CHECK(cstring("abcdef").find_first_not_of("abef", 4) == cstring::npos);

    CHECK(cstring("abcdef").find_first_not_of(nullptr, 1, 2) == 1);
    CHECK(cstring("abcdef").find_first_not_of("abef", 0, 2) == 2);
    CHECK(cstring("abcdef").find_first_not_of("abef", 1, 2) == 2);
    CHECK(cstring("abcdef").find_first_not_of("abef", 2, 2) == 2);
    CHECK(cstring("abcdef").find_first_not_of("abef", 3, 2) == 3);
    CHECK(cstring("abcdef").find_first_not_of("abef", 4, 2) == 4);

    CHECK(cstring("abcdef").find_first_not_of('a') == 1);
    CHECK(cstring("abcdef").find_first_not_of('a', 0) == 1);
    CHECK(cstring("abcdef").find_first_not_of('a', 1) == 1);
  }

  SECTION("find_last_of") {
    CHECK(cstring("abcdef").find_last_of(cstring("cd")) == 3);
    CHECK(cstring("abcdef").find_last_of(cstring("cd"), 5) == 3);
    CHECK(cstring("abcdef").find_last_of(cstring("cd"), 4) == 3);
    CHECK(cstring("abcdef").find_last_of(cstring("cd"), 3) == 3);
    CHECK(cstring("abcdef").find_last_of(cstring("cd"), 2) == 2);
    CHECK(cstring("abcdef").find_last_of(cstring("cd"), 1) == cstring::npos);

    CHECK(cstring("abcdef").find_last_of(nullptr) == cstring::npos);
    CHECK(cstring("abcdef").find_last_of("") == cstring::npos);
    CHECK(cstring("abcdef").find_last_of("cd") == 3);

    CHECK(cstring("abcdef").find_last_of(nullptr, 5) == cstring::npos);
    CHECK(cstring("abcdef").find_last_of("cd", 5) == 3);
    CHECK(cstring("abcdef").find_last_of("cd", 4) == 3);
    CHECK(cstring("abcdef").find_last_of("cd", 3) == 3);
    CHECK(cstring("abcdef").find_last_of("cd", 2) == 2);
    CHECK(cstring("abcdef").find_last_of("cd", 1) == cstring::npos);

    CHECK(cstring("abcdef").find_last_of(nullptr, 5, 1) == cstring::npos);
    CHECK(cstring("abcdef").find_last_of("cd", 5, 1) == 2);
    CHECK(cstring("abcdef").find_last_of("cd", 4, 1) == 2);
    CHECK(cstring("abcdef").find_last_of("cd", 3, 1) == 2);
    CHECK(cstring("abcdef").find_last_of("cd", 2, 1) == 2);
    CHECK(cstring("abcdef").find_last_of("cd", 1, 1) == cstring::npos);

    CHECK(cstring("abcdef").find_last_of('c') == 2);
    CHECK(cstring("abcdef").find_last_of('c', 5) == 2);
    CHECK(cstring("abcdef").find_last_of('c', 4) == 2);
    CHECK(cstring("abcdef").find_last_of('c', 3) == 2);
    CHECK(cstring("abcdef").find_last_of('c', 2) == 2);
    CHECK(cstring("abcdef").find_last_of('c', 1) == cstring::npos);
  }


  SECTION("find_last_not_of") {
    CHECK(cstring("abcdef").find_last_not_of(cstring("abef")) == 3);
    CHECK(cstring("abcdef").find_last_not_of(cstring("abef"), 5) == 3);
    CHECK(cstring("abcdef").find_last_not_of(cstring("abef"), 4) == 3);
    CHECK(cstring("abcdef").find_last_not_of(cstring("abef"), 3) == 3);
    CHECK(cstring("abcdef").find_last_not_of(cstring("abef"), 2) == 2);
    CHECK(cstring("abcdef").find_last_not_of(cstring("abef"), 1) == cstring::npos);

    CHECK(cstring("abcdef").find_last_not_of(nullptr) == 5);
    CHECK(cstring("abcdef").find_last_not_of("") == 5);
    CHECK(cstring("abcdef").find_last_not_of("abef") == 3);

    CHECK(cstring("abcdef").find_last_not_of(nullptr, 4) == 4);
    CHECK(cstring("abcdef").find_last_not_of("abef", 5) == 3);
    CHECK(cstring("abcdef").find_last_not_of("abef", 4) == 3);
    CHECK(cstring("abcdef").find_last_not_of("abef", 3) == 3);
    CHECK(cstring("abcdef").find_last_not_of("abef", 2) == 2);
    CHECK(cstring("abcdef").find_last_not_of("abef", 1) == cstring::npos);

    CHECK(cstring("abcdef").find_last_not_of(nullptr, 4, 1) == 4);
    CHECK(cstring("abcdef").find_last_not_of("abef", 5, 1) == 5);
    CHECK(cstring("abcdef").find_last_not_of("abef", 4, 1) == 4);
    CHECK(cstring("abcdef").find_last_not_of("abef", 3, 1) == 3);
    CHECK(cstring("abcdef").find_last_not_of("abef", 2, 1) == 2);
    CHECK(cstring("abcdef").find_last_not_of("abef", 1, 1) == 1);
    CHECK(cstring("abcdef").find_last_not_of("abef", 0, 1) == cstring::npos);

    CHECK(cstring("abcdef").find_last_not_of('f') == 4);
    CHECK(cstring("abcdef").find_last_not_of('f', 5) == 4);
    CHECK(cstring("abcdef").find_last_not_of('f', 4) == 4);
    CHECK(cstring("abcdef").find_last_not_of('f', 3) == 3);
    CHECK(cstring("abcdef").find_last_not_of('f', 2) == 2);
    CHECK(cstring("abcdef").find_last_not_of('f', 1) == 1);
    CHECK(cstring("abcdef").find_last_not_of('f', 0) == 0);
  }

  SECTION("contains") {
    const char *aux1 = "cde";
    CHECK(cstring("abcdef").contains(cstring::basic_cstring_view(aux1)));
    CHECK(cstring("abcdef").contains('b'));
    CHECK(cstring("abcdef").contains("cde"));

    const char *aux2 = "xyz";
    CHECK(!cstring("abcdef").contains(cstring::basic_cstring_view(aux2)));
    CHECK(!cstring("abcdef").contains('x'));
    CHECK(!cstring("abcdef").contains("xyz"));
  }

  SECTION("ltrim") {
    CHECK(cstring("").ltrim() == std::string(""));
    CHECK(cstring(" ").ltrim() == std::string(""));
    CHECK(cstring("  ").ltrim() == std::string(""));
    CHECK(cstring("abc").ltrim() == std::string("abc"));
    CHECK(cstring(" abc").ltrim() == std::string("abc"));
    CHECK(cstring("  abc").ltrim() == std::string("abc"));
    CHECK(cstring("abc ").ltrim() == std::string("abc "));
  }

  SECTION("rtrim") {
    CHECK(cstring("").rtrim() == std::string(""));
    CHECK(cstring(" ").rtrim() == std::string(""));
    CHECK(cstring("  ").rtrim() == std::string(""));
    CHECK(cstring("abc").rtrim() == std::string("abc"));
    CHECK(cstring("abc ").rtrim() == std::string("abc"));
    CHECK(cstring("abc  ").rtrim() == std::string("abc"));
    CHECK(cstring(" abc").rtrim() == std::string(" abc"));
  }

  SECTION("trim") {
    CHECK(cstring("").trim() == std::string(""));
    CHECK(cstring(" ").trim() == std::string(""));
    CHECK(cstring("  ").trim() == std::string(""));
    CHECK(cstring("abc").trim() == std::string("abc"));
    CHECK(cstring(" abc").trim() == std::string("abc"));
    CHECK(cstring("abc ").trim() == std::string("abc"));
    CHECK(cstring(" abc ").trim() == std::string("abc"));
    CHECK(cstring("  abc  ").trim() == std::string("abc"));
  }

  SECTION("cstring comp cstring") {
    CHECK(cstring("abc") == cstring("abc"));
    CHECK(cstring("abc") != cstring("xyz"));
    CHECK(cstring("abc") < cstring("xyz"));
    CHECK(cstring("abc") <= cstring("abc"));
    CHECK(cstring("abc") <= cstring("xyz"));
    CHECK(cstring("xyz") > cstring("abc"));
    CHECK(cstring("abc") >= cstring("abc"));
    CHECK(cstring("xyz") >= cstring("abc"));
  }

  SECTION("cstring comp char array") {
    CHECK(cstring("abc") == "abc");
    CHECK(cstring("abc") != "xyz");
    CHECK(cstring("abc") < "xyz");
    CHECK(cstring("abc") <= "abc");
    CHECK(cstring("abc") <= "xyz");
    CHECK(cstring("xyz") > "abc");
    CHECK(cstring("abc") >= "abc");
    CHECK(cstring("xyz") >= "abc");
  }

  SECTION("char array comp cstring") {
    CHECK("abc" == cstring("abc"));
    CHECK(!("abc" == cstring("xyz")));
    CHECK("abc" != cstring("xyz"));
    CHECK(!("abc" != cstring("abc")));
    CHECK("abc" < cstring("xyz"));
    CHECK(!("xyz" < cstring("abc")));
    CHECK(!("abc" < cstring("abc")));
    CHECK("abc" <= cstring("abc"));
    CHECK("abc" <= cstring("xyz"));
    CHECK(!("xyz" <= cstring("abc")));
    CHECK("xyz" > cstring("abc"));
    CHECK(!("abc" > cstring("xyz")));
    CHECK(!("abc" > cstring("abc")));
    CHECK("abc" >= cstring("abc"));
    CHECK("xyz" >= cstring("abc"));
    CHECK(!("abc" >= cstring("xyz")));
  }

  SECTION("std::swap") {
    cstring str1("abc");
    cstring str2("xyz");
    std::swap(str1, str2);
    CHECK(str1 == "xyz");
    CHECK(str2 == "abc");
  }

  SECTION("output stream operator") {
    std::stringstream ss;
    ss << cstring("abc");
    CHECK(ss.str() == std::string("abc"));
    ss.str("");
    ss.clear();
    ss << cstring();
    CHECK(ss.str().empty());
  }

  SECTION("std::hash") {
    CHECK(std::hash<cstring>{}(cstring()) != 0);
    CHECK(std::hash<cstring>{}(cstring("abc")) != 0);
    CHECK(std::hash<cstring>{}(cstring("abc")) == std::hash<cstring>{}(cstring("abc")));
    CHECK(std::hash<cstring>{}(cstring("abc")) == std::hash<string>{}(std::string("abc")));
  }

}
