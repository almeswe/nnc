#include "nnc_test.h"

void nnc_test_stats() {
    fprintf(stdout, RESET);
    fprintf(stdout, "[tests: %d, passed: %d, failed: %d] completion: %d%%\n", 
        tests, tests_passed, tests - tests_passed, (int)(tests_passed / (double)tests * 100));
}

void nnc_run_test(nnc_test_fn* fn) {
    pid_t pid = fork();
    if (pid == 0) {
        exit(fn());    
    }
    tests++;
    int status = 0;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == EXIT_FAILURE || WIFSIGNALED(status)) {
        if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            NNC_TEST_PRINTF(BOLDRED "RECEIVED SIGNAL: [%d]\n", sig);
        }
        NNC_TEST_PRINTF(BOLDRED "%s\n", "FAILED");
    }
    else {
        tests_passed++;
        NNC_TEST_PRINTF(BOLDGREEN "%s\n", "PASSED");
    }
}