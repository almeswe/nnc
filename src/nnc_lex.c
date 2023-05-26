#include "nnc_lex.h"

/**
 * @brief Stores string representation of each nnc_tok_kind.
 */
static const char* nnc_tok_strs[] = {
    [TOK_AMPERSAND]     = "TOK_AMPERSAND",
    [TOK_AND]           = "TOK_AND",
    [TOK_ASSIGN]        = "TOK_ASSIGN",
    [TOK_ASTERISK]      = "TOK_ASTERISK",
    [TOK_ATPERSAND]     = "TOK_ATPERSAND",
    [TOK_CBRACE]        = "TOK_CBRACE",
    [TOK_CHR]           = "TOK_CHR",
    [TOK_STR]           = "TOK_STR",
    [TOK_CBRACKET]      = "TOK_CBRACKET",
    [TOK_CIRCUMFLEX]    = "TOK_CIRCUMFLEX",
    [TOK_COLON]         = "TOK_COLON",
    [TOK_COMMA]         = "TOK_COMMA",
    [TOK_CPAREN]        = "TOK_CPAREN",
    [TOK_DOLLAR]        = "TOK_DOLLAR",
    [TOK_DOT]           = "TOK_DOT",
    [TOK_EOF]           = "TOK_EOF",
    [TOK_EQ]            = "TOK_EQ",
    [TOK_EXCMARK]       = "TOK_EXCMARK",
    [TOK_GRAVE]         = "TOK_GRAVE",
    [TOK_GT]            = "TOK_GT",
    [TOK_GTE]           = "TOK_GTE",
    [TOK_IDENT]         = "TOK_IDENT",
    [TOK_LT]            = "TOK_LT",
    [TOK_LTE]           = "TOK_LTE",
    [TOK_LSHIFT]        = "TOK_LSHIFT",
    [TOK_MINUS]         = "TOK_MINUS",
    [TOK_NEQ]           = "TOK_NEQ",
    [TOK_NUMBER]        = "TOK_NUMBER",
    [TOK_OBRACE]        = "TOK_OBRACE",
    [TOK_OBRACKET]      = "TOK_OBRACKET",
    [TOK_OPAREN]        = "TOK_OPAREN",
    [TOK_OR]            = "TOK_OR",
    [TOK_PERCENT]       = "TOK_PERCENT",
    [TOK_PLUS]          = "TOK_PLUS",
    [TOK_QUESTION]      = "TOK_QUESTION",
    [TOK_QUOTE]         = "TOK_QUOTE",
    [TOK_QUOTES]        = "TOK_QUOTES",
    [TOK_RSHIFT]        = "TOK_RSHIFT",
    [TOK_SEMICOLON]     = "TOK_SEMICOLON",
    [TOK_SIGN]          = "TOK_SIGN",
    [TOK_SLASH]         = "TOK_SLASH",
    [TOK_STRING]        = "TOK_STRING",
    [TOK_TILDE]         = "TOK_TILDE",
    [TOK_UNDERSCORE]    = "TOK_UNDERSCORE",
    [TOK_VLINE]         = "TOK_VLINE",
    // keywords
    [TOK_BREAK]         = "TOK_BREAK",
    [TOK_CASE]          = "TOK_CASE",
    [TOK_CAST]          = "TOK_CAST",
    [TOK_CONTINUE]      = "TOK_CONTINUE",
    [TOK_DEFAULT]       = "TOK_DEFAULT",
    [TOK_ENUM]          = "TOK_ENUM",
    [TOK_ELIF]          = "TOK_ELIF",
    [TOK_ENT]           = "TOK_ENT",
    [TOK_EXT]           = "TOK_EXT",
    [TOK_FOR]           = "TOK_FOR",
    [TOK_FN]            = "TOK_FN",
    [TOK_FROM]          = "TOK_FROM",
    [TOK_F32]           = "TOK_F32",
    [TOK_F64]           = "TOK_F64",
    [TOK_GOTO]          = "TOK_GOTO",
    [TOK_IF]            = "TOK_IF",
    [TOK_I8]            = "TOK_I8",
    [TOK_I16]           = "TOK_I16",
    [TOK_I32]           = "TOK_I32",
    [TOK_I64]           = "TOK_I64",
    [TOK_IMPORT]        = "TOK_IMPORT",
    [TOK_NAMESPACE]    = "TOK_NAMESPACE",
    [TOK_PUB]           = "TOK_PUB",
    [TOK_RETURN]        = "TOK_RETURN",
    [TOK_STRUCT]        = "TOK_STRUCT",
    [TOK_SWITCH]        = "TOK_SWITCH",
    [TOK_SIZEOF]        = "TOK_SIZEOF",
    [TOK_TYPEDEF]       = "TOK_TYPEDEF",
    [TOK_UNION]         = "TOK_UNION",
    [TOK_U8]            = "TOK_U8",
    [TOK_U16]           = "TOK_U16",
    [TOK_U32]           = "TOK_U32",
    [TOK_U64]           = "TOK_U64",
    [TOK_LET]           = "TOK_LET",
    [TOK_LABEL]         = "TOK_LABEL",
    [TOK_LENGTHOF]      = "TOK_LENGTHOF",
    [TOK_VAR]           = "TOK_VAR",
    [TOK_VOID]          = "TOK_VOID",
    [TOK_WHILE]         = "TOK_WHILE",
    [TOK_DO]            = "TOK_DO",
    [TOK_ELSE]          = "TOK_ELSE"
};

/**
 * @brief Stores keywords supported by nnc.
 */
static const char* nnc_keywords[] = {
    "break", "case", "cast", "continue",
    "default", "enum", "elif", "ent", "ext",
    "for", "fn", "from", "f32", "f64", "goto",
    "if", "i8", "i16", "i32", "i64", "import",
    "namespace", "pub", "return", "struct", 
    "switch", "sizeof", "typedef", "union", 
    "u8", "u16", "u32", "u64", "let", "label",
    "lengthof", "var", "void", "while", "do", "else"
};

/**
 * @brief Map containing string representation of each keyword
 *  with corresponding nnc_tok_kind. This is needed for fast identifier check.  
*/
static _map_(char*, nnc_tok_kind) nnc_keywods_map = NULL;

static void nnc_lex_undo(nnc_lex* lex) {
    ungetc(lex->cc, lex->fp);
    lex->cc = lex->pc;
}

static char nnc_lex_grab(nnc_lex* lex) {
    if (lex->cc != EOF) {
        lex->pc = lex->cc;
        lex->cc = fgetc(lex->fp);
    }
    return lex->cc;
}

static void nnc_lex_skip(nnc_lex* lex) {
    while (nnc_lex_grab(lex), lex->cc != EOF) {
        switch (lex->cc) {
            case ' ':  case '\a':
            case '\b': case '\f':
            case '\t': case '\v':
            case '\r':
                break;
            case '\n':
                lex->cctx.hint_ln++;
                lex->cctx.hint_ch = 0;
                break;
            default:
                return;
        }
    }
}

static bool nnc_lex_match(nnc_lex* lex, char c) {
    return lex->cc == c;
}

static bool nnc_lex_not_match(nnc_lex* lex, char c) {
    return lex->cc != c;
}

static void nnc_lex_tok_clean(nnc_lex* lex) {
    lex->ctok.size = 0;
    memset(lex->ctok.lexeme, 0, NNC_TOK_BUF_MAX);
}

static void nnc_lex_tok_put_c(nnc_lex* lex, char c) {
    nnc_u64* size = &lex->ctok.size;
    lex->ctok.lexeme[(*size)++] = c;
} 

static void nnc_lex_tok_put_cc(nnc_lex* lex) {
    nnc_lex_tok_put_c(lex, lex->cc);
}

static void nnc_lex_make_recovery(nnc_lex* lex) {
    while (nnc_lex_grab(lex), lex->cc != EOF) {
        if (lex->cc == '\n') {
            break;
        }
    }
}

static char nnc_lex_make_esc(nnc_lex* lex) {
    switch (lex->cc) {
        case 'a':   return '\a';
        case 'b':   return '\b';
        case 'f':   return '\f';
        case 't':   return '\t';
        case 'v':   return '\v';
        case 'r':   return '\r';
        case 'n':   return '\n';
        case '\\':  return '\\';
        case '\'':  return '\'';
        case '\"':  return '\"';
    }
    THROW(NNC_LEX_BAD_ESC, "expected valid escape sequence character.\n");
    // unreachable code part, just to avoid compiler warning
    return '\0';
}

static nnc_tok_kind nnc_lex_grab_chr(nnc_lex* lex) {
    nnc_lex_tok_clean(lex);
    char initial = nnc_lex_grab(lex); 
    if (initial != EOF) {
        if (nnc_lex_not_match(lex, '\\')) {
            nnc_lex_tok_put_c(lex, initial);
        }
        else {
            nnc_lex_grab(lex);
            char esc = nnc_lex_make_esc(lex);
            nnc_lex_tok_put_c(lex, esc);
        }
        nnc_lex_grab(lex);
    }
    if (nnc_lex_not_match(lex, '\'')) {
        THROW(NNC_LEX_BAD_CHR, "expected single quote [\'].\n");
    }
    return TOK_CHR;
}

static nnc_tok_kind nnc_lex_grab_str(nnc_lex* lex) {
    nnc_lex_tok_clean(lex);
    while (nnc_lex_grab(lex) != EOF) {
        if (nnc_lex_match(lex, '\n') || 
            nnc_lex_match(lex, '\"')) {
            break;
        }
        lex->cctx.hint_ch++;
        if (nnc_lex_not_match(lex, '\\')) {
            nnc_lex_tok_put_cc(lex);
        }
        else {
            nnc_lex_grab(lex);
            char esc = nnc_lex_make_esc(lex);
            nnc_lex_tok_put_c(lex, esc);
        }
    }
    if (nnc_lex_not_match(lex, '\"')) {
        THROW(NNC_LEX_BAD_STR, "expected double quote [\"].\n");
    }
    return TOK_STR;
}

static nnc_tok_kind nnc_lex_grab_number(nnc_lex* lex) {
    nnc_lex_tok_clean(lex);
    nnc_lex_tok_put_cc(lex);
    nnc_bool dot_met = false;
    while (nnc_lex_grab(lex) != EOF) {
        if (lex->cc < '0' || lex->cc > '9') {
            if (lex->cc != '.' || dot_met) {
                break;
            }
            dot_met = true;
        }
        lex->cctx.hint_ch++;
        nnc_lex_tok_put_cc(lex);
    }
    nnc_lex_undo(lex);
    return TOK_NUMBER;
}

static nnc_bool nnc_lex_is_keyword(nnc_lex* lex) {
    return map_has_s(nnc_keywods_map, lex->ctok.lexeme);
}

static nnc_tok_kind nnc_lex_get_keyword_kind(nnc_lex* lex) {
    return (nnc_tok_kind)map_get_s(nnc_keywods_map, lex->ctok.lexeme); 
} 

static nnc_tok_kind nnc_lex_grab_ident(nnc_lex* lex) {
    nnc_lex_tok_clean(lex);
    nnc_lex_tok_put_cc(lex);
    while (nnc_lex_grab(lex) != EOF) {
        if ((lex->cc < '0' || lex->cc > '9') &&
            (lex->cc < 'a' || lex->cc > 'z') &&
            (lex->cc < 'A' || lex->cc > 'Z') && lex->cc != '_') {
            break;
        }
        lex->cctx.hint_ch++;
        nnc_lex_tok_put_cc(lex);
    }
    nnc_lex_undo(lex);
    if (nnc_lex_is_keyword(lex)) {
        return nnc_lex_get_keyword_kind(lex);
    }
    return TOK_IDENT;
}

static nnc_bool nnc_lex_adjust(nnc_lex* lex, char c) {
    if (lex->cc == EOF) {
        return false;
    }
    char adjusted = nnc_lex_grab(lex);
    if (adjusted != c) {
        nnc_lex_undo(lex);
    } 
    return adjusted == c;
}

nnc_tok_kind nnc_lex_next(nnc_lex* lex) {
    TRY {
        nnc_lex_skip(lex);
        switch (lex->cc) {
            case EOF:   NNC_LEX_COMMIT(TOK_EOF);
            case '*':   NNC_LEX_COMMIT(TOK_ASTERISK);
            case '@':   NNC_LEX_COMMIT(TOK_ATPERSAND);
            case '}':   NNC_LEX_COMMIT(TOK_CBRACE);
            case ']':   NNC_LEX_COMMIT(TOK_CBRACKET);
            case '^':   NNC_LEX_COMMIT(TOK_CIRCUMFLEX);
            case ':':   NNC_LEX_COMMIT(TOK_COLON);
            case ',':   NNC_LEX_COMMIT(TOK_COMMA);
            case ')':   NNC_LEX_COMMIT(TOK_CPAREN);
            case '$':   NNC_LEX_COMMIT(TOK_DOLLAR);
            case '.':   NNC_LEX_COMMIT(TOK_DOT);
            case '`':   NNC_LEX_COMMIT(TOK_GRAVE);
            case '-':   NNC_LEX_COMMIT(TOK_MINUS);
            case '{':   NNC_LEX_COMMIT(TOK_OBRACE);
            case '[':   NNC_LEX_COMMIT(TOK_OBRACKET);
            case '(':   NNC_LEX_COMMIT(TOK_OPAREN);
            case '%':   NNC_LEX_COMMIT(TOK_PERCENT);
            case '+':   NNC_LEX_COMMIT(TOK_PLUS);
            case '?':   NNC_LEX_COMMIT(TOK_QUESTION);
            case ';':   NNC_LEX_COMMIT(TOK_SEMICOLON);
            case '#':   NNC_LEX_COMMIT(TOK_SIGN);
            case '/':   NNC_LEX_COMMIT(TOK_SLASH);
            case '~':   NNC_LEX_COMMIT(TOK_TILDE);
            
            case '<':   NNC_LEX_ADJUST('=') ? NNC_LEX_SET_TERN(TOK_LTE)     : 
                        NNC_LEX_ADJUST('<') ? NNC_LEX_SET_TERN(TOK_LSHIFT)  : NNC_LEX_SET_TERNB(TOK_LT);

            case '>':   NNC_LEX_ADJUST('=') ? NNC_LEX_SET_TERN(TOK_GTE)     :
                        NNC_LEX_ADJUST('>') ? NNC_LEX_SET_TERN(TOK_RSHIFT)  : NNC_LEX_SET_TERNB(TOK_GT);

            case '=':   NNC_LEX_ADJUST('=') ? NNC_LEX_SET_TERN(TOK_EQ)  : NNC_LEX_SET_TERNB(TOK_ASSIGN);
            case '!':   NNC_LEX_ADJUST('=') ? NNC_LEX_SET_TERN(TOK_NEQ) : NNC_LEX_SET_TERNB(TOK_EXCMARK);
            case '&':   NNC_LEX_ADJUST('&') ? NNC_LEX_SET_TERN(TOK_AND) : NNC_LEX_SET_TERNB(TOK_AMPERSAND);
            case '|':   NNC_LEX_ADJUST('|') ? NNC_LEX_SET_TERN(TOK_OR)  : NNC_LEX_SET_TERNB(TOK_VLINE);
                break;
            case '\'':  NNC_LEX_COMMIT(nnc_lex_grab_chr(lex));
            case '\"':  NNC_LEX_COMMIT(nnc_lex_grab_str(lex));
            case '1': case '2':
            case '3': case '4':
            case '5': case '6':
            case '7': case '8':
            case '9': case '0': 
                NNC_LEX_COMMIT(nnc_lex_grab_number(lex));
                break;
            case '_':
            case 'a': case 'A': case 'b': case 'B': 
            case 'c': case 'C': case 'd': case 'D':
            case 'e': case 'E': case 'f': case 'F':
            case 'g': case 'G': case 'h': case 'H':
            case 'i': case 'I': case 'j': case 'J':
            case 'k': case 'K': case 'l': case 'L':
            case 'm': case 'M': case 'n': case 'N':
            case 'o': case 'O': case 'p': case 'P':
            case 'q': case 'Q': case 'r': case 'R':
            case 's': case 'S': case 't': case 'T':
            case 'u': case 'U': case 'v': case 'V':
            case 'w': case 'W': case 'x': case 'X':
            case 'y': case 'Y': case 'z': case 'Z':
                NNC_LEX_COMMIT(nnc_lex_grab_ident(lex));
                break;
        }
        ETRY;
        return lex->ctok.kind;
    }
    CATCHALL {
        nnc_error(sformat("%s: %s", CATCHED.repr, CATCHED.what), &lex->cctx);
        nnc_lex_make_recovery(lex);
        return nnc_lex_next(lex);
    }
}

const char* nnc_tok_str(nnc_tok_kind kind) {
    return nnc_tok_strs[kind];
}

void nnc_lex_init(nnc_lex* out_lex, const char* fpath) {
    out_lex->cc = '\0';
    out_lex->fp = fopen(fpath, "r");
    if (out_lex->fp == NULL) {
        THROW(NNC_LEX_BAD_FILE, fpath);
    }
    out_lex->fpath = fpath;
    out_lex->cctx.fabs = fpath;
    if (nnc_keywods_map == NULL) {
        // initialize map of keywords, for fast identifier check
        nnc_keywods_map = map_init();
        const nnc_u64 nnc_keywords_size = sizeof(nnc_keywords)/sizeof(*nnc_keywords);
        for (nnc_u64 i = 0ull; i < nnc_keywords_size; i++) {
            nnc_tok_kind kind = (nnc_tok_kind)(TOK_BREAK + i);
            map_put_s(nnc_keywods_map, nnc_keywords[i], kind);
        }
        assert(nnc_keywods_map->len == nnc_keywords_size);
    }
}   

void nnc_lex_fini(nnc_lex* lex) {
    if (lex != NULL) {
        fclose(lex->fp);
        memset(lex, 0, sizeof(nnc_lex));
    }
}

void nnc_lex_keywords_map_fini() {
    if (nnc_keywods_map != NULL) {
        map_fini(nnc_keywods_map);
        nnc_keywods_map = NULL;
    }
}