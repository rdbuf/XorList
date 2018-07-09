[![Build Status](https://travis-ci.org/rdbuf/XorList.svg?branch=master)](https://travis-ci.org/rdbuf/XorList)
[![Header](https://img.shields.io/badge/single%20header-master-blue.svg)](https://github.com/rdbuf/XorList/blob/master/include/XorList.hpp)

XOR linked list: [wikipedia](https://en.wikipedia.org/wiki/XOR_linked_list).

Aimed at a partial implementation of std::list as specified in [n4659](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf).

The minimum supported standard is C++17.

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
