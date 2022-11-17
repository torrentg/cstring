CXXFLAGS= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wnull-dereference -Weffc++

all: example tests coverage

example: cstring-example.cpp
	$(CXX) -O2 $(CXXFLAGS) -o cstring-example cstring-example.cpp
	./cstring-example

tests: cstring-tests.cpp
	$(CXX) -g $(CXXFLAGS) -o cstring-tests cstring-tests.cpp
	./cstring-tests

coverage: cstring-tests.cpp
	$(CXX) --coverage -O0 $(CXXFLAGS) -o cstring-coverage cstring-tests.cpp -lgcov
	./cstring-coverage
	mkdir coverage
	lcov --no-external -d . -o coverage/coverage.info -c
	lcov --remove coverage/coverage.info '*/doctest.h' -o coverage/coverage.info
	lcov --remove coverage/coverage.info '*/cstring-tests.cpp' -o coverage/coverage.info
	genhtml -o coverage coverage/coverage.info

clean: 
	rm -f cstring-tests
	rm -f cstring-coverage
	rm -f cstring-example
	rm -f *.gcda *.gcno
	rm -rvf coverage
