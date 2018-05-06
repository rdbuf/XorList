[![Build Status](https://travis-ci.org/rdbuf/XorList.svg?branch=master)](https://travis-ci.org/rdbuf/XorList)

XOR linked list: [wikipedia](https://en.wikipedia.org/wiki/XOR_linked_list)

This implementation is by no means a drop-in replacement for std::list, yet replicates a part of it.

Theoretically speaking, it supports C++14, although tested against C++17.

CMakeLists.txt defines a XorList INTERFACE target.

Tests dependencies:
- C++17-compatible Clang or GCC
- CMake
- googletest (obtained by means of CMake)
- git
- (optional) LLVM toolchain

Compile & run tests:
```
mkdir build && cd build
cmake .. && make
tests/run
```

In case of compiling with Clang and having the LLVM toolchain installed, you can additionally request the coverage analysis:
```
tests/coverage.sh
```
