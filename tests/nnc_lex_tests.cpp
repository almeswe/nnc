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
    auto chr_toks = std::vector<char> {
        'a', 'b', 'c'
    };
    auto str_toks = std::vector<const char*> {
        "", "1", "123", "test string abc"
    };
    auto num_toks = std::vector<const char*> {
        "1", "23", "456", "7890", "1.0",
        "23.23", "456.456", "7890.7890"
    };
    auto ident_toks = std::vector<const char*> {
        "_", "___", "_______identifier", "__id__",
        "test", "t123", "_909test"
    };
    for (size_t i = 0; i < chr_toks.size(); i++) {
        EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
        EXPECT_EQ(lex.ctok.size, 1);
        EXPECT_EQ(lex.ctok.lexeme[0], chr_toks[i]);
    }
    for (size_t i = 0; i < str_toks.size(); i++) {
        EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
        EXPECT_EQ(lex.ctok.size, strlen(str_toks[i]));
        EXPECT_STREQ(lex.ctok.lexeme, str_toks[i]);
    }
    for (size_t i = 0; i < ident_toks.size(); i++) {
        EXPECT_EQ(nnc_lex_next(&lex), TOK_IDENT);
        EXPECT_EQ(lex.ctok.size, strlen(ident_toks[i]));
        EXPECT_STREQ(lex.ctok.lexeme, ident_toks[i]);
    }
    for (size_t i = 0; i < num_toks.size(); i++) {
        EXPECT_EQ(nnc_lex_next(&lex), TOK_NUMBER);
        EXPECT_EQ(lex.ctok.size, strlen(num_toks[i]));
        EXPECT_STREQ(lex.ctok.lexeme, num_toks[i]);
    }
    EXPECT_EQ(nnc_lex_next(&lex), TOK_EOF);
    nnc_lex_fini(&lex);
}

TEST(nnc_lex_test, nnc_lex_next_3) {
    nnc_lex_init(&lex, NNC_LEX_ALL_ESC_TOKS_FILE);
    auto chr_toks = std::vector<char> {
        '\a', '\b', '\f', '\t', '\v',
        '\r', '\n', '\\', '\'', '\"'
    };
    auto str_toks = std::vector<const char*> {
        "\a\b\f\t\v\r\n\\\'\"",
        "\'s\'\n\rtext continuation",
        "GET / HTTP/1.1\r\n"
    };
    for (size_t i = 0; i < chr_toks.size(); i++) {
        EXPECT_EQ(nnc_lex_next(&lex), TOK_CHR);
        EXPECT_EQ(lex.ctok.size, 1);
        EXPECT_EQ(lex.ctok.lexeme[0], chr_toks[i]);
    }
    for (size_t i = 0; i < str_toks.size(); i++) {
        EXPECT_EQ(nnc_lex_next(&lex), TOK_STR);
        EXPECT_EQ(lex.ctok.size, strlen(lex.ctok.lexeme));
        EXPECT_STREQ(lex.ctok.lexeme, str_toks[i]);
    }
    EXPECT_EQ(nnc_lex_next(&lex), TOK_EOF);
    nnc_lex_fini(&lex);
}