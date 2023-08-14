#include "../nnc_test.hpp"

TEST(nnc_lex_tests, keyword_separation) {
    nnc_arena_init(&glob_arena);
    nnc_lex lex = { 0 };
    contents = 
        "as\n"
        "break\n"
        "case\n"
        "cast\n"
        "continue\n"
        "default\n"
        "enum\n"
        "elif\n"
        "ent\n"
        "ext\n"
        "for\n"
        "fn\n"
        "from\n"
        "f32\n"
        "f64\n"
        "goto\n"
        "if\n"
        "i8\n"
        "i16\n"
        "i32\n"
        "i64\n"
        "import\n"
        "namespace\n"
        "pub\n"
        "return\n"
        "struct\n"
        "switch\n"
        "sizeof\n"
        "type\n"
        "union\n"
        "u8\n"
        "u16\n"
        "u32\n"
        "u64\n"
        "let\n"
        "label\n"
        "lengthof\n"
        "var\n"
        "void\n"
        "while\n"
        "do\n"
        "else\n"
    ;
    FILE* fp = nnc_create_test_file(contents);
    nnc_lex_init_with_fp(&lex, fp);
    ASSERT_TRUE(fp != NULL);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AS);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_BREAK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CASE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CAST);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CONTINUE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DEFAULT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ENUM);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ELIF);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EXT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_FOR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_FN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_FROM);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_F32);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_F64);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_GOTO);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IF);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_I8);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_I16);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_I32);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_I64);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IMPORT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_NAMESPACE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_PUB);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_RETURN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_STRUCT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SWITCH);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SIZEOF);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_TYPE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_UNION);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_U8);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_U16);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_U32);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_U64);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_LET);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_LABEL);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_LENGTHOF);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_VAR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_VOID);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_WHILE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DO);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ELSE);
    fclose(fp);
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
}