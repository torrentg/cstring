# cstring

**This project is under development.**

cstring is a C++ immutable C-string (aka const char array) with reference counting.

It is a pointer to chars where the pointed memory is prefixed by the ref-counter 
(4-bytes) and the string length (4-bytes).

Main advantages are :

* `sizeof(cstring) = 8`
* Reference counting integrated (forget object ownership)
* Reduce the number of indirections (allocated memory includes the ref-counter)
* Includes string length harnessing the full power of `string_view`
* Easy debug (object points to string content)
* Mimics the interface of a `const std::string` (eg. `find_first_not_of()`)
* Additional utility functions (`contains()`, `trim()`, `ltrim()`, `rtrim()`)

Main drawbacks:

* Immutable content (we cannot change the string content)
* 8 additional bytes are allocated (ref-counter + length + eventual padding)
* No Small String Optimization (SSO)
* String length and ref-count limited to `4'294'967'295`
* Stateful allocators (`sizeof(allocator) > 0`) not supported

## Usage

Drop off [`cstring.hpp`](cstring.hpp) in your project and start using the cstring.

Read [`example.cpp`](example.cpp) file to see how to use it.

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

## Authors

* **Gerard Torrent** - _Initial work_ - [torrentg](https://github.com/torrentg/)

## License

This project is licensed under the GNU LGPL v3 License - see the [LICENSE](LICENSE) file for details.
