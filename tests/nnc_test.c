#include "nnc_test.h"

void nnc_test_stats() {
    fprintf(stdout, RESET BOLDYELLOW);
    fprintf(stdout, "tests: %d, passed: %d, failed: %d\n", 
        tests, tests_passed, tests - tests_passed);
}

void nnc_run_test(nnc_test_fn* fn) {
    pid_t pid = fork();
    if (pid == 0) {
        exit(fn());    
    }
    tests++;
    int status = 0;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == EXIT_SUCCESS) {
        tests_passed++;
        NNC_TEST_PRINTF(BOLDGREEN "%s\n", "PASSED");
    }
    else {
        NNC_TEST_PRINTF(BOLDRED "%s\n", "FAILED");
    }
}