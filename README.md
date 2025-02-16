# cstring

`cstring` is a C++ immutable C-string (aka `const char array`) with reference counting.

It is a pointer to chars where the pointed memory is prefixed by the reference counter
(4 bytes) and the string length (4 bytes).

Features:

* It is a `const char *` having `sizeof(cstring) = 8`.
* Behaves like a `std::string`.
* Integrated reference counting reduces the number of indirections.
* Includes string length, harnessing the full power of `string_view`.
* Easy debug (object points to string content).
* Additional utility functions (`contains()`, `trim()`, etc).
* A comparator (`cstring_compare`) is provided for the most common types.

Main drawbacks:

* Immutable content (the string content cannot be changed).
* 8 additional bytes are allocated (reference counter + length + eventual padding).
* No Small String Optimization (SSO).
* String length and reference counter are limited to `4'294'967'295`.
* Stateful allocators (`sizeof(allocator) > 0`) are not supported.

## Usage

Drop [`cstring.hpp`](cstring.hpp) into your project and start using `cstring`.

Read the [`cstring-example.cpp`](cstring-example.cpp) file to see how to use it.

```c++
#include "cstring.hpp"
...
// just use it like a const std::string
gto::cstring str1 = "hello world!"
std::cout << "Length of '" << str << "' is " << str.length() << std::endl;
```

## Testing

```bash
# example
make example

# unit tests
make tests

# code coverage
make coverage
firefox coverage/index.html &
```

## Contributors

| Name | Contribution |
|:-----|:-------------|
| [Gerard Torrent](https://github.com/torrentg/) | Initial work<br/>Code maintainer|
| [Matthieu M.](https://codereview.stackexchange.com/users/8999/matthieu-m) | [Code review](https://codereview.stackexchange.com/questions/281365/an-immutable-c-string-with-ref-counting) |

## License

This project is licensed under the GNU LGPL v3 License - see the [LICENSE](LICENSE) file for details.
