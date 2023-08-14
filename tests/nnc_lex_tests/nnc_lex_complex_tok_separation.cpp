#include "../nnc_test.hpp"

TEST(nnc_lex_tests, complex_tok_separation) {
    nnc_arena_init(&glob_arena);
    nnc_lex lex = { 0 };
    contents = 
        "\'a\' \'b\' \'c\'\n"
        "\"\" \"1\" \"123\" \"test string abc\"\n"
        "_ ___ _______identifier\n"
        "__id__ test t123 _909test\n"
        "1 23 456 7890\n"
        "1.0 23.23 456.456 7890.7890\n"

        "0 00 123\n"
        "0x33 0xabcdef 0xABCDEF\n"
        "0o777\n"
        "0b01010100010100\n"
        "0\n"

        "4.1e-3\n"
        "5.0e+10\n"
        "3.40282346638528859811704183484516925e+40f32\n"
        "2.3E+15f32\n"
        "2.3f64\n"
        "0u64\n"
        "0x8000000000000000U64\n"
        "2.33f64\n"
        "0x7fffffffffffffffi64\n"
        "123I16\n"
        "0xfffu64\n"
        "2.2f32\n"
        "0.0f64\n"
    ;

    auto fp = nnc_create_test_file(contents);
    nnc_lex_init_with_fp(&lex, fp);
    auto chr_toks = std::vector<char> {
        'a', 'b', 'c'
    };
    auto str_toks = std::vector<const char*> {
        "", "1", "123", "test string abc"
    };
    auto num_toks = std::vector<const char*> {
        "1", "23", "456", "7890", "1.0",
        "23.23", "456.456", "7890.7890",
        "0", "00", "123",
        "x33", "xabcdef", "xABCDEF",
        "o777",
        "b01010100010100",
        "0",
        "4.1e-3",
        "5.0e+10",
        "3.40282346638528859811704183484516925e+40f32",
        "2.3E+15f32",
        "2.3f64",
        "0u64",
        "x8000000000000000U64",
        "2.33f64",
        "x7fffffffffffffffi64",
        "123I16",
        "xfffu64",
        "2.2f32",
        "0.0f64"
    };
    auto ident_toks = std::vector<const char*>{
        "_", "___", "_______identifier", "__id__",
        "test", "t123", "_909test"
    };
    for (size_t i = 0; i < chr_toks.size(); i++) {
        ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CHR);
        ASSERT_TRUE(lex.ctok.size == 1);
        ASSERT_TRUE(lex.ctok.lexeme[0] == chr_toks[i]);
    }
    for (size_t i = 0; i < str_toks.size(); i++) {
        ASSERT_TRUE(nnc_lex_next(&lex) == TOK_STR);
        ASSERT_TRUE(lex.ctok.size == strlen(str_toks[i]));
        ASSERT_TRUE(strcmp(lex.ctok.lexeme, str_toks[i]) == 0);
    }
    for (size_t i = 0; i < ident_toks.size(); i++) {
        ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
        ASSERT_TRUE(lex.ctok.size == strlen(ident_toks[i]));
        ASSERT_TRUE(strcmp(lex.ctok.lexeme, ident_toks[i]) == 0);
    }
    for (size_t i = 0; i < num_toks.size(); i++) {
        ASSERT_TRUE(nnc_lex_next(&lex) == TOK_NUMBER);
        ASSERT_TRUE(lex.ctok.size == strlen(num_toks[i]));
        ASSERT_TRUE(strcmp(lex.ctok.lexeme, num_toks[i]) == 0);
    }
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EOF);
    fclose(fp);
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
}