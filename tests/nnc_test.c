#include "nnc_test.h"

void nnc_test_stats() {
    fprintf(stdout, RESET);
    fprintf(stdout, "[tests: %d, passed: %d, failed: %d] completion: %d%%\n", 
        tests, tests_passed, tests - tests_passed, (int)(tests_passed / (double)tests * 100));
}

void nnc_run_tests(const nnc_test* first, const nnc_test* last) {
    for (const nnc_test* test = first; test < last; test++) {
        printf(BOLDMAGENTA "(%3d) %s:%s\n" RED, ++tests, 
            test->test_suit_name, test->test_case_name);
        nnc_run_test(test);
    }
    nnc_test_stats();
}

void nnc_run_test(const nnc_test* test) {
    pid_t pid = fork();
    if (pid == 0) {
        test->test(), exit(EXIT_SUCCESS);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (WEXITSTATUS(status) == EXIT_FAILURE || WIFSIGNALED(status)) {
        if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf(BOLDRED "RECEIVED SIGNAL: [%d]\n" RED, sig);
        }
        printf(BOLDRED "%s\n" RED, "FAILED");
    }
    else {
        tests_passed++;
        printf(BOLDGREEN "%s\n" RED, "PASSED");
    }
}