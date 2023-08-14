#include "../nnc_test.hpp"

TEST(nnc_lex_tests, incorrect_file_catch) {
    nnc_lex lex = { 0 };
    TRY {
        nnc_lex_init(&lex, "______NON_EXISTENT_PATH_____");
        ASSERT_TRUE(false);
    }
    CATCH (NNC_LEX_BAD_FILE) {
        ASSERT_TRUE(true);
    }
}