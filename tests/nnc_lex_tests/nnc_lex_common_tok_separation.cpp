#include "../nnc_test.hpp"

TEST(nnc_lex_tests, common_tok_separation) {
    nnc_arena_init(&glob_arena);
    nnc_lex lex = { 0 };

    contents =
        "& * @ } ] ^ : , ) $ . = ! ` > < - { "
        "[ ( % + ? ; # / ~ | == != || && >= <= << >> ::"
    ; 

    FILE* fp = nnc_create_test_file(contents);
    nnc_lex_init_with_fp(&lex, fp);
    ASSERT_TRUE(fp != NULL);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AMPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASTERISK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ATPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACKET);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CIRCUMFLEX);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COLON);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DOLLAR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DOT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASSIGN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EXCMARK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_GRAVE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_GT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_LT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_MINUS);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACKET);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_PERCENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_PLUS);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_QUESTION);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SIGN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SLASH);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_TILDE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_VLINE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EQ);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_NEQ);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_GTE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_LTE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_LSHIFT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_RSHIFT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DCOLON);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EOF);
    fclose(fp);
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
}