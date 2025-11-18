/*
 * Test Runner
 *
 * Runs all test suites for Card Fifty-Two.
 * Exit code 0 = all tests passed, 1 = some tests failed.
 */

#include "test.h"

// Define test statistics globals (declared as extern in test.h)
int g_tests_run = 0;
int g_tests_passed = 0;
int g_tests_failed = 0;

// Forward declarations
void run_struct_tests(void);
void run_state_tests(void);
void run_trinket_tests(void);
void run_targeting_tests(void);
void run_doubled_integration_tests(void);
void run_combat_start_tests(void);
void run_stats_tests(void);

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                                                           ║\n");
    printf("║            Card Fifty-Two Test Suite                     ║\n");
    printf("║            Constitutional Compliance Checks              ║\n");
    printf("║                                                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Reset global counters
    g_tests_run = 0;
    g_tests_passed = 0;
    g_tests_failed = 0;

    // Run all test suites (counters accumulate across suites)
    run_struct_tests();
    run_state_tests();
    run_trinket_tests();
    run_targeting_tests();
    run_doubled_integration_tests();
    run_combat_start_tests();
    run_stats_tests();

    // Save final totals
    int total_run = g_tests_run;
    int total_passed = g_tests_passed;
    int total_failed = g_tests_failed;

    // Print final summary
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                     FINAL RESULTS                         ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");

    if (total_failed == 0) {
        printf("║  " COLOR_GREEN "✓ ALL TESTS PASSED" COLOR_RESET "                                      ║\n");
        printf("║    Total: %d tests                                       ║\n", total_run);
    } else {
        printf("║  " COLOR_RED "✗ SOME TESTS FAILED" COLOR_RESET "                                   ║\n");
        printf("║    Passed: %d/%d                                         ║\n", total_passed, total_run);
        printf("║    Failed: %d/%d                                         ║\n", total_failed, total_run);
    }

    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");

    TEST_EXIT();
}
