#include "nnc_test.hpp"

nnc_arena glob_arena;
nnc_error_canarie glob_error_canarie;
nnc_exception_stack glob_exception_stack;
nnc_deferred_stack glob_deferred_stack;

#include "./nnc_buf_tests/nnc_buf_basic.cpp"
#include "./nnc_buf_tests/nnc_buf_free.cpp"
#include "./nnc_buf_tests/nnc_buf_push_n_fetch_1k.cpp"
#include "./nnc_buf_tests/nnc_buf_push_n_fetch_50k.cpp"

#include "./nnc_lex_tests/nnc_lex_incorrect_file.cpp"
#include "./nnc_lex_tests/nnc_lex_keyword_separation.cpp"
#include "./nnc_lex_tests/nnc_lex_mixed_tok_separation.cpp"
#include "./nnc_lex_tests/nnc_lex_common_tok_separation.cpp"
#include "./nnc_lex_tests/nnc_lex_escape_tok_separation.cpp"
#include "./nnc_lex_tests/nnc_lex_complex_tok_separation.cpp"

#include "./nnc_map_tests/nnc_map_dups.cpp"
#include "./nnc_map_tests/nnc_map_fini.cpp"
#include "./nnc_map_tests/nnc_map_basic.cpp"
#include "./nnc_map_tests/nnc_map_push_n_fetch_n_pop_1k.cpp"
#include "./nnc_map_tests/nnc_map_push_n_fetch_n_pop_50k.cpp"

#include "./nnc_try_catch_tests/nnc_try_catch_nest_1.cpp"
#include "./nnc_try_catch_tests/nnc_try_catch_nest_2.cpp"
#include "./nnc_try_catch_tests/nnc_try_catch_nest_3.cpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}