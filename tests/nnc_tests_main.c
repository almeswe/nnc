#include "nnc_test.h"

TEST_SECTION_BEGIN;

nnc_arena glob_arena;
nnc_error_canarie glob_error_canarie;
nnc_exception_stack glob_exception_stack;
nnc_deferred_stack glob_deferred_stack;

#include "nnc_lex_tests.c"
#include "nnc_map_tests.c"
#include "nnc_buf_tests.c"
#include "nnc_try_catch_tests.c"

TEST_SECTION_END;

int main(int argc, char** argv) {
    RUN_ALL_TESTS();
    return EXIT_SUCCESS;
}