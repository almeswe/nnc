#include "nnc_test.hpp"

#include "./nnc_lex_tests/nnc_lex_mock_test.cpp"

nnc_arena glob_arena;
nnc_error_canarie glob_error_canarie;
nnc_exception_stack glob_exception_stack;
nnc_deferred_stack glob_deferred_stack;

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}