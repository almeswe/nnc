#include "gtest/gtest.h"

#include "../src/nnc_lex.h"
#include "../src/nnc_lex.c"

#define NNC_LEX_ALL_DEF_TOKS_FILE "nnc_lex_test_1"
#define NNC_LEX_ALL_SPC_TOKS_FILE "nnc_lex_test_2"
#define NNC_LEX_ALL_ESC_TOKS_FILE "nnc_lex_test_3"

static nnc_lex lex = { 0 };

TEST(nnc_lex_test, nnc_lex_next_1) {
    nnc_lex_init(&lex, NNC_LEX_ALL_DEF_TOKS_FILE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_AMPERSAND);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_ASTERISK);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_ATPERSAND);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_CBRACE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_CBRACKET);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_CIRCUMFLEX);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_COLON);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_COMMA);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_CPAREN);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_DOLLAR);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_DOT);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_EQUALS);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_EXCMARK);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_GRAVE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_GT);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_LT);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_MINUS);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_OBRACE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_OBRACKET);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_OPAREN);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_PERCENT);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_PLUS);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_QUESTION);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_SEMICOLON);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_SIGN);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_SLASH);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_TILDE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_VLINE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_EOF);
    nnc_lex_fini(&lex);
}

TEST(nnc_lex_test, nnc_lex_next_2) {
    nnc_lex_init(&lex, NNC_LEX_ALL_SPC_TOKS_FILE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1ull);
    EXPECT_EQ(lex.ctok.lexeme[0], 'a');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1ull);
    EXPECT_EQ(lex.ctok.lexeme[0], 'b');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1ull);
    EXPECT_EQ(lex.ctok.lexeme[0], 'c');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 0);
    EXPECT_STREQ(lex.ctok.lexeme, "");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_STREQ(lex.ctok.lexeme, "1");
    
    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 3);
    EXPECT_STREQ(lex.ctok.lexeme, "123");
    
    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 15);
    EXPECT_STREQ(lex.ctok.lexeme, "test string abc");
    
    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_STREQ(lex.ctok.lexeme, "_");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 3);
    EXPECT_STREQ(lex.ctok.lexeme, "___");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 17);
    EXPECT_STREQ(lex.ctok.lexeme, "_______identifier");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 6);
    EXPECT_STREQ(lex.ctok.lexeme, "__id__");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 4);
    EXPECT_STREQ(lex.ctok.lexeme, "test");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 4);
    EXPECT_STREQ(lex.ctok.lexeme, "t123");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
    EXPECT_EQ(lex.ctok.size, 8);
    EXPECT_STREQ(lex.ctok.lexeme, "_909test");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_STREQ(lex.ctok.lexeme, "1");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 2);
    EXPECT_STREQ(lex.ctok.lexeme, "23");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 3);
    EXPECT_STREQ(lex.ctok.lexeme, "456");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 4);
    EXPECT_STREQ(lex.ctok.lexeme, "7890");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 3);
    EXPECT_STREQ(lex.ctok.lexeme, "1.0");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 5);
    EXPECT_STREQ(lex.ctok.lexeme, "23.23");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 7);
    EXPECT_STREQ(lex.ctok.lexeme, "456.456");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
    EXPECT_EQ(lex.ctok.size, 9);
    EXPECT_STREQ(lex.ctok.lexeme, "7890.7890");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_EOF);
    nnc_lex_fini(&lex);
}

TEST(nnc_lex_test, nnc_lex_next_3) {
    nnc_lex_init(&lex, NNC_LEX_ALL_ESC_TOKS_FILE);
    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\a');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\b');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\f');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\t');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\v');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\r');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\n');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\\');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\'');

    EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
    EXPECT_EQ(lex.ctok.size, 1);
    EXPECT_EQ(lex.ctok.lexeme[0], '\"');
    
    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 10);
    EXPECT_STREQ(lex.ctok.lexeme, "\a\b\f\t\v\r\n\\\'\"");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 22);
    EXPECT_STREQ(lex.ctok.lexeme, "\'s\'\n\rtext continuation");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
    EXPECT_EQ(lex.ctok.size, 16);
    EXPECT_STREQ(lex.ctok.lexeme, "GET / HTTP/1.1\r\n");

    EXPECT_EQ(nnc_lex_next(&lex), TOK_EOF);
    nnc_lex_fini(&lex);
}