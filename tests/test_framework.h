#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, msg); \
            return false; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            printf("  [PASS] %s\n", #test_func); \
            tests_passed++; \
        } else { \
            tests_failed++; \
        } \
        tests_run++; \
    } while (0)

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#endif // TEST_FRAMEWORK_H
