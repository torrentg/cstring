#include <map>
#include <iostream>
#include <algorithm>
#include "cstring.hpp"

// g++ -std=c++17 -DNDEBUG -o cstring-example cstring-example.cpp 

using namespace std;
using namespace gto;

int main()
{
    cstring str1 = "hello world!";

    cout << "str = " << str1 << endl;
    cout << "size = " << str1.size() << endl;
    cout << "find('w') = " << std::find(str1.cbegin(), str1.cend(), 'w') << endl;
    cout << "substr(0, 5) = " << str1.substr(0, 5) << endl;
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
    cout << "last occurrence of char in [aeiou] is at position " << str1.find_last_of("aeiou") << endl;
    cout << "last occurrence of char not in [orld!] is at position " << str1.find_last_not_of("orld!") << endl;

    cstring str2 = "   str   ";

    cout << "content = '" << str2 << "'" << endl;
    cout << "ltrim = '" << str2.ltrim() << "'" << endl;
    cout << "rtrim = '" << str2.rtrim() << "'" << endl;
    cout << "trim = '" << str2.trim() << "'" << endl;
    cout << "'" << str2 << "' is " << (str2 < str1 ? "": "not ") << "less than '" << str1 << "'" << endl;

    cstring x1;
    cout << "x1.use_count() = " << x1.use_count() << endl;
    x1 = "foo";
    cout << "x1.use_count() = " << x1.use_count() << endl;

    {
        cstring x2 = x1;
        cout << "x1.use_count() = " << x1.use_count() << endl;
        cout << "&x2 = " << static_cast<const void*>(&x2) << ", "
             << "x2.data = " << x2 << ", "
             << "&x2.data = " << static_cast<const void*>(x2.data()) << endl;
    }

    cout << "x1.use_count() = " << x1.use_count() << endl;
    cout << "&x1 = " << static_cast<const void*>(&x1) << ", "
         << "x1.data = " << x1 << ", "
         << "&x1.data = " << static_cast<const void*>(x1.data()) << endl;

    std::map<cstring, int, cstring_compare> cstring_map;
    cstring_map[cstring("apple")] = 1;
    cstring_map[cstring("banana")] = 2;

    // searching a value in the map using a distinct type
    cout << "find(\"apple\") = " << cstring_map.find(static_cast<const char *>("apple"))->second << endl;
    cout << "find(std::string(\"banana\")) = " << cstring_map.find(std::string("banana"))->second << endl;
}
