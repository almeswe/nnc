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

#include "./nnc_literal_tests/nnc_i8_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_u8_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_i16_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_u16_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_i32_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_u32_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_i64_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_u64_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_f32_literal_tests.cpp"
#include "./nnc_literal_tests/nnc_f64_literal_tests.cpp"

#include "./nnc_semantic_tests/nnc_resolve_plus_expr_test.cpp"
#include "./nnc_semantic_tests/nnc_resolve_minus_expr_test.cpp"
#include "./nnc_semantic_tests/nnc_resolve_deref_expr_test.cpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}