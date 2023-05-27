#include "nnc_test.h"

static nnc_lex lex;

#define NNC_LEX_ALL_DEF_TOKS_FILE   "nnc_lex_test_1"
#define NNC_LEX_ALL_SPC_TOKS_FILE   "nnc_lex_test_2"
#define NNC_LEX_ALL_ESC_TOKS_FILE   "nnc_lex_test_3"
#define NNC_LEX_TOKS_FILE           "nnc_lex_test_4"
#define NNC_LEX_KEYWORDS_TEST_FILE  "nnc_lex_keywords_test"

TEST(test_1, nnclex) {
    nnc_arena_init(&glob_arena);
    nnc_lex_init(&lex, NNC_LEX_ALL_DEF_TOKS_FILE);
    assert(nnc_lex_next(&lex) == TOK_AMPERSAND);
    assert(nnc_lex_next(&lex) == TOK_ASTERISK);
    assert(nnc_lex_next(&lex) == TOK_ATPERSAND);
    assert(nnc_lex_next(&lex) == TOK_CBRACE);
    assert(nnc_lex_next(&lex) == TOK_CBRACKET);
    assert(nnc_lex_next(&lex) == TOK_CIRCUMFLEX);
    assert(nnc_lex_next(&lex) == TOK_COLON);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_DOLLAR);
    assert(nnc_lex_next(&lex) == TOK_DOT);
    assert(nnc_lex_next(&lex) == TOK_ASSIGN);
    assert(nnc_lex_next(&lex) == TOK_EXCMARK);
    assert(nnc_lex_next(&lex) == TOK_GRAVE);
    assert(nnc_lex_next(&lex) == TOK_GT);
    assert(nnc_lex_next(&lex) == TOK_LT);
    assert(nnc_lex_next(&lex) == TOK_MINUS);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);
    assert(nnc_lex_next(&lex) == TOK_OBRACKET);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_PERCENT);
    assert(nnc_lex_next(&lex) == TOK_PLUS);
    assert(nnc_lex_next(&lex) == TOK_QUESTION);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);
    assert(nnc_lex_next(&lex) == TOK_SIGN);
    assert(nnc_lex_next(&lex) == TOK_SLASH);
    assert(nnc_lex_next(&lex) == TOK_TILDE);
    assert(nnc_lex_next(&lex) == TOK_VLINE);
    assert(nnc_lex_next(&lex) == TOK_EQ);
    assert(nnc_lex_next(&lex) == TOK_NEQ);
    assert(nnc_lex_next(&lex) == TOK_OR);
    assert(nnc_lex_next(&lex) == TOK_AND);
    assert(nnc_lex_next(&lex) == TOK_GTE);
    assert(nnc_lex_next(&lex) == TOK_LTE);
    assert(nnc_lex_next(&lex) == TOK_LSHIFT);
    assert(nnc_lex_next(&lex) == TOK_RSHIFT);
    assert(nnc_lex_next(&lex) == TOK_EOF);
    nnc_lex_fini(&lex);
    nnc_lex_keywords_map_fini();
    nnc_arena_fini(&glob_arena);
    assert(glob_arena.alloc_bytes == 0);
}

TEST(test_2, nnclex) {
    nnc_arena_init(&glob_arena);
    nnc_lex_init(&lex, NNC_LEX_ALL_SPC_TOKS_FILE);
    char chr_toks[] = {
        'a', 'b', 'c'
    };
    char* str_toks[] = {
        "", "1", "123", "test string abc"
    };
    char* num_toks[] = {
        "1", "23", "456", "7890", "1.0",
        "23.23", "456.456", "7890.7890"
    };
    char* ident_toks[] = {
        "_", "___", "_______identifier", "__id__",
        "test", "t123", "_909test"
    };
    for (size_t i = 0; i < sizeof(chr_toks)/sizeof(char); i++) {
        assert(nnc_lex_next(&lex) == TOK_CHR);
        assert(lex.ctok.size == 1);
        assert(lex.ctok.lexeme[0] == chr_toks[i]);
    }
    for (size_t i = 0; i < sizeof(str_toks)/sizeof(*str_toks); i++) {
        assert(nnc_lex_next(&lex) == TOK_STR);
        assert(lex.ctok.size == strlen(str_toks[i]));
        assert(strcmp(lex.ctok.lexeme, str_toks[i]) == 0);
    }
    for (size_t i = 0; i < sizeof(ident_toks)/sizeof(*ident_toks); i++) {
        assert(nnc_lex_next(&lex) == TOK_IDENT);
        assert(lex.ctok.size == strlen(ident_toks[i]));
        assert(strcmp(lex.ctok.lexeme, ident_toks[i]) == 0);
    }
    for (size_t i = 0; i < sizeof(num_toks)/sizeof(*num_toks); i++) {
        assert(nnc_lex_next(&lex) == TOK_NUMBER);
        assert(lex.ctok.size == strlen(num_toks[i]));
        assert(strcmp(lex.ctok.lexeme, num_toks[i]) == 0);
    }
    assert(nnc_lex_next(&lex) == TOK_EOF);
    nnc_arena_fini(&glob_arena);
}

TEST(test_3, nnclex) {
    nnc_lex_init(&lex, NNC_LEX_ALL_ESC_TOKS_FILE);
    char chr_toks[] = {
        '\a', '\b', '\f', '\t', '\v',
        '\r', '\n', '\\', '\'', '\"'
    };
    char* str_toks[] = {
        "\a\b\f\t\v\r\n\\\'\"",
        "\'s\'\n\rtext continuation",
        "GET / HTTP/1.1\r\n"
    };
    for (size_t i = 0; i < sizeof(chr_toks)/sizeof(char); i++) {
        assert(nnc_lex_next(&lex) == TOK_CHR);
        assert(lex.ctok.size == 1);
        assert(lex.ctok.lexeme[0] == chr_toks[i]);
    }
    for (size_t i = 0; i < sizeof(str_toks)/sizeof(*str_toks); i++) {
        assert(nnc_lex_next(&lex) == TOK_STR);
        assert(lex.ctok.size == strlen(lex.ctok.lexeme));
        assert(strcmp(lex.ctok.lexeme, str_toks[i]) == 0);
    }
    assert(nnc_lex_next(&lex) == TOK_EOF);
    nnc_lex_fini(&lex);
}

TEST(test_4, nnclex) {
    TRY {
        nnc_lex_init(&lex, "asldasjkdjasdkasjd");
        assert(false);
    }
    CATCH (NNC_LEX_BAD_FILE) {
        assert(true);
    }
}

TEST(test_5, nnclex) {
    nnc_arena_init(&glob_arena);
    nnc_lex_init(&lex, NNC_LEX_TOKS_FILE);
    assert(nnc_lex_next(&lex) == TOK_SIGN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_STR);
    
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);
    
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_ASTERISK);
    assert(nnc_lex_next(&lex) == TOK_ASTERISK);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_ASSIGN);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);
    assert(nnc_lex_next(&lex) == TOK_NUMBER);
    assert(nnc_lex_next(&lex) == TOK_CBRACE);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_AMPERSAND);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OBRACKET);
    assert(nnc_lex_next(&lex) == TOK_NUMBER);
    assert(nnc_lex_next(&lex) == TOK_CBRACKET);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_FOR);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_ASSIGN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_AMPERSAND);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_STR);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_AMPERSAND);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_DOT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_IF);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_EQ);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);
    assert(nnc_lex_next(&lex) == TOK_BREAK);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);
    assert(nnc_lex_next(&lex) == TOK_CBRACE);

    assert(nnc_lex_next(&lex) == TOK_CBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_CBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_STR);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_DOT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_DOT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_CBRACE);

    assert(nnc_lex_next(&lex) == TOK_RETURN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_CBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_ASTERISK);
    assert(nnc_lex_next(&lex) == TOK_ASTERISK);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_OBRACE);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_AMPERSAND);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_ASSIGN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_COMMA);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_OPAREN);
    assert(nnc_lex_next(&lex) == TOK_AMPERSAND);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_CPAREN);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_RETURN);
    assert(nnc_lex_next(&lex) == TOK_IDENT);
    assert(nnc_lex_next(&lex) == TOK_SEMICOLON);

    assert(nnc_lex_next(&lex) == TOK_CBRACE);

    assert(nnc_lex_next(&lex) == TOK_EOF);
    nnc_arena_fini(&glob_arena);
}

TEST(keywords_test, nnclex) {
    nnc_arena_init(&glob_arena);
    nnc_lex_init(&lex, NNC_LEX_KEYWORDS_TEST_FILE);
    assert(nnc_lex_next(&lex) == TOK_BREAK);
    assert(nnc_lex_next(&lex) == TOK_CASE);
    assert(nnc_lex_next(&lex) == TOK_CAST);
    assert(nnc_lex_next(&lex) == TOK_CONTINUE);
    assert(nnc_lex_next(&lex) == TOK_DEFAULT);
    assert(nnc_lex_next(&lex) == TOK_ENUM);
    assert(nnc_lex_next(&lex) == TOK_ELIF);
    assert(nnc_lex_next(&lex) == TOK_ENT);
    assert(nnc_lex_next(&lex) == TOK_EXT);
    assert(nnc_lex_next(&lex) == TOK_FOR);
    assert(nnc_lex_next(&lex) == TOK_FN);
    assert(nnc_lex_next(&lex) == TOK_FROM);
    assert(nnc_lex_next(&lex) == TOK_F32);
    assert(nnc_lex_next(&lex) == TOK_F64);
    assert(nnc_lex_next(&lex) == TOK_GOTO);
    assert(nnc_lex_next(&lex) == TOK_IF);
    assert(nnc_lex_next(&lex) == TOK_I8);
    assert(nnc_lex_next(&lex) == TOK_I16);
    assert(nnc_lex_next(&lex) == TOK_I32);
    assert(nnc_lex_next(&lex) == TOK_I64);
    assert(nnc_lex_next(&lex) == TOK_IMPORT);
    assert(nnc_lex_next(&lex) == TOK_NAMESPACE);
    assert(nnc_lex_next(&lex) == TOK_PUB);
    assert(nnc_lex_next(&lex) == TOK_RETURN);
    assert(nnc_lex_next(&lex) == TOK_STRUCT);
    assert(nnc_lex_next(&lex) == TOK_SWITCH);
    assert(nnc_lex_next(&lex) == TOK_SIZEOF);
    assert(nnc_lex_next(&lex) == TOK_TYPEDEF);
    assert(nnc_lex_next(&lex) == TOK_UNION);
    assert(nnc_lex_next(&lex) == TOK_U8);
    assert(nnc_lex_next(&lex) == TOK_U16);
    assert(nnc_lex_next(&lex) == TOK_U32);
    assert(nnc_lex_next(&lex) == TOK_U64);
    assert(nnc_lex_next(&lex) == TOK_LET);
    assert(nnc_lex_next(&lex) == TOK_LABEL);
    assert(nnc_lex_next(&lex) == TOK_LENGTHOF);
    assert(nnc_lex_next(&lex) == TOK_VAR);
    assert(nnc_lex_next(&lex) == TOK_VOID);
    assert(nnc_lex_next(&lex) == TOK_WHILE);
    assert(nnc_lex_next(&lex) == TOK_DO);
    assert(nnc_lex_next(&lex) == TOK_ELSE);
    nnc_arena_fini(&glob_arena);
}