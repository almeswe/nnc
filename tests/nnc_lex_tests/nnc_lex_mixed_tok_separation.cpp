#include "../nnc_test.hpp"

TEST(nnc_lex_tests, mixed_tok_separation) {
    nnc_arena_init(&glob_arena);
    nnc_lex lex = { 0 };
    contents = 
        "#include \"nnc_core.h\"\n"
        "nnc_arena glob_arena;\n"
        "nnc_exception_stack glob_exception_stack;\n"
        "static nnc_i32 nnc_main(nnc_i32 argc, char** argv) {\n"
        "    nnc_lex l = { 0 };\n"
        "    TRY {\n"
        "        nnc_lex_init(&l, argv[1]);\n"
        "        for (;;) {\n"
        "            nnc_tok_kind tok = nnc_lex_next(&l);\n"
        "            printf(\"%s: %s\\n\", nnc_tok_str(tok), nnc_ctx_tostr(&l.cctx));\n"
        "            if (tok == TOK_EOF) {\n"
        "                break;\n"
        "            }\n"
        "        }\n"
        "        ETRY;\n"
        "    }\n"
        "    CATCH (NNC_LEX_BAD_FILE) {\n"
        "        nnc_abort(sformat(\"%s: %s\\n\", CATCHED.repr,\n"
        "            CATCHED.what), NULL);\n"
        "    }\n"
        "    return EXIT_SUCCESS;\n"
        "}\n"
        "nnc_i32 main(nnc_i32 argc, char** argv) {\n"
        "    nnc_arena_init(&glob_arena);\n"
        "    nnc_i32 status = nnc_main(argc, argv);\n"
        "    nnc_arena_fini(&glob_arena);\n"
        "    return status;\n"
        "}"
    ;
    FILE* fp = nnc_create_test_file(contents);
    nnc_lex_init_with_fp(&lex, fp);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SIGN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_STR);
    
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);
    
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASTERISK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASTERISK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASSIGN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_NUMBER);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AMPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACKET);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_NUMBER);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACKET);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_FOR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASSIGN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AMPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_STR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AMPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DOT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IF);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EQ);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_BREAK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_STR);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DOT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_DOT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_RETURN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASTERISK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASTERISK);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AMPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_ASSIGN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_COMMA);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_OPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_AMPERSAND);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CPAREN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_RETURN);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_IDENT);
    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_SEMICOLON);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_CBRACE);

    ASSERT_TRUE(nnc_lex_next(&lex) == TOK_EOF);
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
}