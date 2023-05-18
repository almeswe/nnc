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

extern nnc_arena glob_arena;

typedef void (nnc_test_fn)();

static nnc_i32 tests = 0;
static nnc_i32 tests_passed = 0;

typedef struct _nnc_test {
    const void* _unused;
    nnc_test_fn* test;
    const char* test_case_name;
    const char* test_suit_name;
} nnc_test;

#define TEST_MUTE 1
#if TEST_MUTE
    #define TEST_PRINTF(fmt, ...) (void)fmt
#else
    #define TEST_PRINTF(fmt, ...)                   \
        fprintf(stdout, RESET CYAN fmt, __VA_ARGS__),   \
        fprintf(stderr, RESET BOLDRED)
#endif

#define TEST_SECTION_ATTR  __attribute__((section("NNC_TEST_SECTION")))
#define TEST_SECTION_BEGIN const nnc_test __start_of_ts TEST_SECTION_ATTR = { NULL, NULL, NULL }
#define TEST_SECTION_END   const nnc_test __end_of_ts   TEST_SECTION_ATTR = { NULL, NULL, NULL }

#define FIRST_TEST  &__start_of_ts + 1
#define LAST_TEST   &__end_of_ts

#define TEST(case, suit)                                                    \
    void test_ ## suit ## _ ## case();                                      \
    const nnc_test test_info_ ## suit ## _ ## case TEST_SECTION_ATTR = {    \
        .test = test_ ## suit ## _ ## case,                                 \
        .test_case_name = #case,                                            \
        .test_suit_name = #suit                                             \
    };                                                                      \
    void test_ ## suit ## _ ## case()

#define RUN_ALL_TESTS() nnc_run_tests(FIRST_TEST, LAST_TEST)

void nnc_test_stats();
void nnc_run_tests(const nnc_test* first, const nnc_test* last);
void nnc_run_test(const nnc_test* test);

#endif