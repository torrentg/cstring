#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include "cstring.hpp"

// g++ -std=c++17 -o cstring-example cstring-example.cpp 

using namespace std;
using namespace gto;

int main() {
  cstring str1 = "hello world!";
  cout << str1 << endl;
  cout << "size = " << str1.size() << endl;
  auto it = std::find(str1.cbegin(), str1.cend(), 'w');
  cout << it << endl;
  auto substr = str1.substr(0, 5);
  cout << substr << endl;
  cout << (str1.contains('w') ? "" : "not ") << "contains 'w'" << endl;
  cout << (str1.contains("hello") ? "" : "not ") << "contains 'hello'" << endl;
  cout << ("hello world!" == str1 ? "" : "not ") << "equals to 'hello world!'" << endl;
  cout << (str1.starts_with("hello") ? "": "not ") << "starting with 'hello'" << endl;
  cout << (str1.ends_with("!") ? "": "not ") << "ending with '!'" << endl;
  cout << "'w' appears at position " << str1.find('w') << endl;
  cout << "'wo' appears at position " << str1.find("wo") << endl;
  cout << "last 'o' appears at position " << str1.rfind('o') << endl;
  cout << "last 'll' appears at position " << str1.rfind("ll") << endl;
  cout << "first char in [a-d] appears at position " << str1.find_first_of("abcd") << endl;
  cout << "first char not in [haeiou] appears at position " << str1.find_first_not_of("haeiou") << endl;
  cout << "last apparison of chars in [aeiou] is at position " << str1.find_last_of("aeiou") << endl;
  cout << "last apparison of chars not in [orld!] is at position " << str1.find_last_not_of("orld!") << endl;

  cstring str2 = "   str   ";
  cout << "content = '" << str2 << "'" << endl;
  cout << "ltrim = '" << str2.ltrim() << "'" << endl;
  cout << "rtrim = '" << str2.rtrim() << "'" << endl;
  cout << "trim = '" << str2.trim() << "'" << endl;
  cout << "'" << str2 << "' is " << (str2 < str1 ? "": "not ") << "less than '" << str1 << "'" << endl;

  cstring x1;
  {
    cstring x2 = "foo";
    cout << "&x2 = " << static_cast<const void*>(&x2) << ", "
         << "x2.data = " << x2 << ", "
         << "&x2.data = " << static_cast<const void*>(x2.data()) << endl;
    x1 = x2;
  }
    cout << "&x1 = " << static_cast<const void*>(&x1) << ", "
         << "x1.data = " << x1 << ", "
         << "&x1.data = " << static_cast<const void*>(x1.data()) << endl;
}
