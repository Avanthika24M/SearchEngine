#define _GNU_SOURCE
#include "search.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int fail_next_write = 1;

int __real_fprintf(FILE *stream, const char *format, ...);

int __wrap_fprintf(FILE *stream, const char *format, ...)
{
    if (fail_next_write && strstr(format, "HSDS2") != NULL) {
        fail_next_write = 0;
        return -1;
    }

    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);
    return result;
}

int main(void)
{
    SearchIndex idx;
    assert(hs_init(&idx) == 0);

    int result = hs_save(&idx, "test");
    assert(result == -1);
    assert(fail_next_write == 0);

    hs_destroy(&idx);

    puts("PASS: storage write failure cleanup");
    return 0;
}