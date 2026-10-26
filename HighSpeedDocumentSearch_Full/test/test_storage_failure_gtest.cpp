#include <gtest/gtest.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>
extern "C" {
#include "search.h"
}

static bool fail_next_header_write = false;

extern "C" int __wrap_fprintf(FILE *stream, const char *format, ...) {
    if (fail_next_header_write && std::strstr(format, "HSDS2") != nullptr) {
        fail_next_header_write = false;
        return -1;
    }
    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);
    return result;
}

TEST(StorageFailureTest, CleansUpWhenHeaderWriteFails) {
    char pattern[] = "/tmp/hs_gtest_fail_XXXXXX";
    char *dir = mkdtemp(pattern);
    ASSERT_NE(dir, nullptr);
    SearchIndex idx{};
    ASSERT_EQ(hs_init(&idx), 0);
    fail_next_header_write = true;
    EXPECT_EQ(hs_save(&idx, dir), -1);
    EXPECT_FALSE(fail_next_header_write);
    std::string temp_path = std::string(dir) + "/documents.tmp";
    EXPECT_NE(access(temp_path.c_str(), F_OK), 0);
    hs_destroy(&idx);
    rmdir(dir);
}
