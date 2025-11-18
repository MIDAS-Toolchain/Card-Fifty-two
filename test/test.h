#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// ============================================================================
// TEST FRAMEWORK MACROS
// ============================================================================

// Test statistics (extern declarations - defined in run_tests.c)
extern int g_tests_run;
extern int g_tests_passed;
extern int g_tests_failed;

// Color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

/**
 * TEST - Define a test function
 * Usage: TEST(test_name) { ... }
 */
#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        g_tests_run++; \
        printf(COLOR_CYAN "  Running: " #name COLOR_RESET "\n"); \
        test_##name(); \
        g_tests_passed++; \
        printf(COLOR_GREEN "  ✓ PASSED: " #name COLOR_RESET "\n"); \
    } \
    static void test_##name(void)

/**
 * RUN_TEST - Execute a test
 */
#define RUN_TEST(name) \
    do { \
        printf("\n"); \
        run_test_##name(); \
    } while(0)

/**
 * ASSERT_EQ - Assert two values are equal
 */
#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            printf(COLOR_RED "  ✗ FAILED: %s:%d - Expected %ld, got %ld" COLOR_RESET "\n", \
                   __FILE__, __LINE__, (long)(expected), (long)(actual)); \
            g_tests_failed++; \
            g_tests_passed--; \
            return; \
        } \
    } while(0)

/**
 * ASSERT_TRUE - Assert condition is true
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf(COLOR_RED "  ✗ FAILED: %s:%d - Expected true, got false: " #condition COLOR_RESET "\n", \
                   __FILE__, __LINE__); \
            g_tests_failed++; \
            g_tests_passed--; \
            return; \
        } \
    } while(0)

/**
 * ASSERT_FALSE - Assert condition is false
 */
#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            printf(COLOR_RED "  ✗ FAILED: %s:%d - Expected false, got true: " #condition COLOR_RESET "\n", \
                   __FILE__, __LINE__); \
            g_tests_failed++; \
            g_tests_passed--; \
            return; \
        } \
    } while(0)

/**
 * ASSERT_NOT_NULL - Assert pointer is not NULL
 */
#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf(COLOR_RED "  ✗ FAILED: %s:%d - Expected non-NULL pointer: " #ptr COLOR_RESET "\n", \
                   __FILE__, __LINE__); \
            g_tests_failed++; \
            g_tests_passed--; \
            return; \
        } \
    } while(0)

/**
 * ASSERT_NULL - Assert pointer is NULL
 */
#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            printf(COLOR_RED "  ✗ FAILED: %s:%d - Expected NULL pointer: " #ptr COLOR_RESET "\n", \
                   __FILE__, __LINE__); \
            g_tests_failed++; \
            g_tests_passed--; \
            return; \
        } \
    } while(0)

/**
 * ASSERT_STR_EQ - Assert two strings are equal
 */
#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            printf(COLOR_RED "  ✗ FAILED: %s:%d - Expected \"%s\", got \"%s\"" COLOR_RESET "\n", \
                   __FILE__, __LINE__, (expected), (actual)); \
            g_tests_failed++; \
            g_tests_passed--; \
            return; \
        } \
    } while(0)

/**
 * TEST_SUITE_BEGIN - Start a test suite
 */
#define TEST_SUITE_BEGIN(name) \
    do { \
        printf("\n" COLOR_YELLOW "═══════════════════════════════════════════════════════════" COLOR_RESET "\n"); \
        printf(COLOR_YELLOW "  Test Suite: " #name COLOR_RESET "\n"); \
        printf(COLOR_YELLOW "═══════════════════════════════════════════════════════════" COLOR_RESET "\n"); \
    } while(0)

/**
 * TEST_SUITE_END - End a test suite and print results
 */
#define TEST_SUITE_END() \
    do { \
        printf("\n" COLOR_YELLOW "═══════════════════════════════════════════════════════════" COLOR_RESET "\n"); \
        if (g_tests_failed == 0) { \
            printf(COLOR_GREEN "  ✓ ALL TESTS PASSED" COLOR_RESET " (%d/%d)\n", g_tests_passed, g_tests_run); \
        } else { \
            printf(COLOR_RED "  ✗ SOME TESTS FAILED" COLOR_RESET " (%d passed, %d failed, %d total)\n", \
                   g_tests_passed, g_tests_failed, g_tests_run); \
        } \
        printf(COLOR_YELLOW "═══════════════════════════════════════════════════════════" COLOR_RESET "\n\n"); \
    } while(0)

/**
 * TEST_EXIT - Exit with status code based on test results
 */
#define TEST_EXIT() \
    exit(g_tests_failed == 0 ? 0 : 1)

#endif // TEST_H
