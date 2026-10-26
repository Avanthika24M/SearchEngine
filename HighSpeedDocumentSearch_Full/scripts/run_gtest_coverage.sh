#!/usr/bin/env bash
set -euo pipefail
mkdir -p build/gtest_100
rm -f build/gtest_100/*.gcda build/gtest_100/*.gcno
CC=${CC:-gcc}
CXX=${CXX:-g++}
COMMON=(-O0 -g --coverage -Iinclude)
"$CC" -std=c11 "${COMMON[@]}" -c src/index.c -o build/gtest_100/index.o
"$CC" -std=c11 "${COMMON[@]}" -c src/storage.c -o build/gtest_100/storage.o
"$CXX" -std=c++17 "${COMMON[@]}" test/test_search_gtest.cpp \
  build/gtest_100/index.o build/gtest_100/storage.o \
  -o build/gtest_100/regular_gtests -lgtest -pthread -lm
"$CXX" -std=c++17 "${COMMON[@]}" test/test_storage_failure_gtest.cpp \
  build/gtest_100/index.o build/gtest_100/storage.o \
  -Wl,--wrap=fprintf -o build/gtest_100/fault_gtests -lgtest_main -lgtest -pthread -lm
./build/gtest_100/regular_gtests
./build/gtest_100/fault_gtests
gcov -b -c -o build/gtest_100/index.gcno src/index.c
gcov -b -c -o build/gtest_100/storage.gcno src/storage.c
