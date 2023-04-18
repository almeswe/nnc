#ifndef _NNC_LEX_H
#define _NNC_LEX_H

#include "nnc_ctx.h"
#include "nnc_arena.h"

#define NNC_TOK_BUF_MAX 512
#define NNC_LEX_COMMIT(k) lex->ctok.kind = k; break

typedef enum _nnc_tok_kind {
    TOK_AMPERSAND, TOK_ASTERISK,
    TOK_ATPERSAND, TOK_CBRACE,
    TOK_CBRACKET,  TOK_CIRCUMFLEX,
    TOK_COLON,     TOK_COMMA,
    TOK_CPAREN,    TOK_DOLLAR, 
    TOK_DOT,       TOK_EOF,
    TOK_EQUALS,    TOK_EXCMARK,
    TOK_GRAVE,     TOK_GT,
    TOK_IDENT,     TOK_LT,
    TOK_MINUS,     TOK_NUMBER,
    TOK_OBRACE,    TOK_OBRACKET,
    TOK_OPAREN,    TOK_PERCENT,
    TOK_PLUS,      TOK_QUESTION,
    TOK_QUOTE,     TOK_QUOTES,
    TOK_SEMICOLON, TOK_SIGN,
    TOK_SLASH,     TOK_STRING,
    TOK_TILDE,     TOK_UNDERSCORE,
    TOK_VLINE
} nnc_tok_kind;

typedef struct _nnc_tok {
    nnc_u64 size;
    nnc_tok_kind kind;
    char lexeme[NNC_TOK_BUF_MAX];
} nnc_tok;

typedef struct _nnc_lex {
    char cc;
    char pc;
    FILE* fp;
    nnc_ctx cctx;
    nnc_tok ctok;
    const char* fpath;
} nnc_lex;

nnc_tok_kind nnc_lex_next(nnc_lex* lex);
const char* nnc_tok_str(nnc_tok_kind kind);
void nnc_lex_init(nnc_lex* out_lex);
void nnc_lex_fini(nnc_lex* lex);

#endif