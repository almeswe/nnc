#include "../nnc_test.hpp"

TEST(nnc_lex_tests, escape_tok_separation) {
    nnc_arena_init(&glob_arena);
    nnc_lex lex = { 0 };
    contents = 
        "\'\\a\' \'\\b\' \'\\f\'\n"
        "\'\\t\' \'\\v\' \'\\r\'\n"
        "\'\\n\' \'\\\\\' \'\\\'\' \'\\\"\'\n"
        "\"\\a\\b\\f\\t\\v\\r\\n\\\\\\'\\\"\"\n"
        "\"\\\'s\\\'\\n\\rtext continuation\"\n"
        "\"GET / HTTP/1.1\\r\\n\"\n"
    ;
    FILE* fp = nnc_create_test_file(contents);
    nnc_lex_init_with_fp(&lex, fp);
    ASSERT_TRUE(fp != NULL);
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
        ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CHR);
        ASSERT_TRUE(lex.ctok.size == 1);
        ASSERT_TRUE(lex.ctok.lexeme[0] == chr_toks[i]);
    }
    for (size_t i = 0; i < str_toks.size(); i++) {
        ASSERT_TRUE(nnc_lex_next(&lex) == TOK_STR);
        ASSERT_TRUE(lex.ctok.size == strlen(lex.ctok.lexeme));
        ASSERT_TRUE(strcmp(lex.ctok.lexeme, str_toks[i]) == 0);
    }
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EOF);
    fclose(fp);
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
}