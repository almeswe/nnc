#include "nnc_lex.h"

// todo: add support for keywords and compound chars

static const char* nnc_tok_strs[] = {
    [TOK_AMPERSAND]     = "TOK_AMPERSAND",
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
    [TOK_EQUALS]        = "TOK_EQUALS",
    [TOK_EXCMARK]       = "TOK_EXCMARK",
    [TOK_GRAVE]         = "TOK_GRAVE",
    [TOK_GT]            = "TOK_GT",
    [TOK_IDENT]         = "TOK_IDENT",
    [TOK_LT]            = "TOK_LT",
    [TOK_MINUS]         = "TOK_MINUS",
    [TOK_NUMBER]        = "TOK_NUMBER",
    [TOK_OBRACE]        = "TOK_OBRACE",
    [TOK_OBRACKET]      = "TOK_OBRACKET",
    [TOK_OPAREN]        = "TOK_OPAREN",
    [TOK_PERCENT]       = "TOK_PERCENT",
    [TOK_PLUS]          = "TOK_PLUS",
    [TOK_QUESTION]      = "TOK_QUESTION",
    [TOK_QUOTE]         = "TOK_QUOTE",
    [TOK_QUOTES]        = "TOK_QUOTES",
    [TOK_SEMICOLON]     = "TOK_SEMICOLON",
    [TOK_SIGN]          = "TOK_SIGN",
    [TOK_SLASH]         = "TOK_SLASH",
    [TOK_STRING]        = "TOK_STRING",
    [TOK_TILDE]         = "TOK_TILDE",
    [TOK_UNDERSCORE]    = "TOK_UNDERSCORE",
    [TOK_VLINE]         = "TOK_VLINE"
};

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
            case ' ':
            case '\a':
            case '\b':
            case '\f':
            case '\t':
            case '\v':
            case '\r':
                break;
            case '\n':
                lex->cctx.hint_line++;
                lex->cctx.hint_char = 0;
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
    // todo: error here
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
        printf("single quote expected, %d %d\n", 
            lex->cctx.hint_line, lex->cctx.hint_char), exit(1);            
    }
    return TOK_CHR;
}

static nnc_tok_kind nnc_lex_grab_str(nnc_lex* lex) {
    nnc_lex_tok_clean(lex);
    // todo: move lex->cctx.hint_char++ to nnc_lex_grab 
    while (nnc_lex_grab(lex) != EOF) {
        if (nnc_lex_match(lex, '\n') || 
            nnc_lex_match(lex, '\"')) {
            break;
        }
        lex->cctx.hint_char++;
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
        printf("double quote expected\n"), exit(1);            
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
        lex->cctx.hint_char++;
        nnc_lex_tok_put_cc(lex);
    }
    nnc_lex_undo(lex);
    return TOK_NUMBER;
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
        lex->cctx.hint_char++;
        nnc_lex_tok_put_cc(lex);
    }
    nnc_lex_undo(lex);
    return TOK_IDENT;
}

nnc_tok_kind nnc_lex_next(nnc_lex* lex) {
    nnc_lex_skip(lex);
    switch (lex->cc) {
        case EOF:   NNC_LEX_COMMIT(TOK_EOF);
        case '&':   NNC_LEX_COMMIT(TOK_AMPERSAND);
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
        case '=':   NNC_LEX_COMMIT(TOK_EQUALS);
        case '!':   NNC_LEX_COMMIT(TOK_EXCMARK);
        case '`':   NNC_LEX_COMMIT(TOK_GRAVE);
        case '>':   NNC_LEX_COMMIT(TOK_GT);
        case '<':   NNC_LEX_COMMIT(TOK_LT);
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
        case '|':   NNC_LEX_COMMIT(TOK_VLINE);
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
    return lex->ctok.kind;
}

const char* nnc_tok_str(nnc_tok_kind kind) {
    return nnc_tok_strs[kind];
}

void nnc_lex_init(nnc_lex* out_lex, const char* fpath) {
    out_lex->cc = '\0';
    out_lex->fpath = fpath;
    out_lex->fp = fopen(out_lex->fpath, "r");
    if (out_lex->fp == NULL) {
        //todo: remove this code
        perror("fopen");
        exit(EXIT_FAILURE);
    }
}   

void nnc_lex_fini(nnc_lex* lex) {
    if (lex != NULL) {
        fclose(lex->fp);
        memset(lex, 0, sizeof(nnc_lex));
    }
}