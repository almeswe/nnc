#ifndef _NNC_TEST_H
#define _NNC_TEST_H

#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../src/nnc_core.h"

#define RESET       "\033[0m"
#define BLACK       "\033[30m"      
#define RED         "\033[31m"      
#define GREEN       "\033[32m"      
#define YELLOW      "\033[33m"      
#define BLUE        "\033[34m"      
#define MAGENTA     "\033[35m"      
#define CYAN        "\033[36m"      
#define WHITE       "\033[37m"      
#define BOLDBLACK   "\033[1m\033[30m"      
#define BOLDRED     "\033[1m\033[31m"      
#define BOLDGREEN   "\033[1m\033[32m"      
#define BOLDYELLOW  "\033[1m\033[33m"
#define BOLDBLUE    "\033[1m\033[34m"
#define BOLDMAGENTA "\033[1m\033[35m"      
#define BOLDCYAN    "\033[1m\033[36m"      
#define BOLDWHITE   "\033[1m\033[37m"      

#define NNC_TEST_PRINTF(fmt, ...)                   \
    fprintf(stdout, RESET CYAN fmt, __VA_ARGS__),   \
    fprintf(stderr, RESET BOLDRED)

#define NNC_ASSERT(expr)                            \
    if (!(expr)) {                                  \
        fprintf(stderr, "Assertion failed: " #expr  \
            " [%s:%d]\n", __FILE__, __LINE__);      \
        return EXIT_FAILURE;                        \
    }

#define NNC_TEST(test)                                              \
    NNC_TEST_PRINTF(BOLDMAGENTA "(%3d) " #test "\n", ++tests),      \
    nnc_run_test(test)

typedef int (nnc_test_fn)();

static nnc_i32 tests = 0;
static nnc_i32 tests_passed = 0;

void nnc_test_stats();
void nnc_run_test(nnc_test_fn* fn);

#endif