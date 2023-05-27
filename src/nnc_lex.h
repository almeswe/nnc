#ifndef _NNC_LEX_H
#define _NNC_LEX_H

#include "nnc_ctx.h"

#include "nnc_map.h"
#include "nnc_try_catch.h"

#define NNC_TOK_BUF_MAX 512

#define NNC_LEX_MATCH(c)     (lex->cc == c)
#define NNC_LEX_NOT_MATCH(c) (lex->cc != c)

#define NNC_TOK_PUT_C(c) (lex->ctok.lexeme[lex->ctok.size] = c, lex->ctok.size++)
#define NNC_TOK_PUT_CC() (NNC_TOK_PUT_C(lex->cc))

#define NNC_IS_KEYWORD()            map_has_s(nnc_keywods_map, lex->ctok.lexeme)
#define NNC_GET_KEYWORD_KIND()      (nnc_tok_kind)map_get_s(nnc_keywods_map, lex->ctok.lexeme);

#define NNC_LEX_ADJUST(c)       nnc_lex_adjust(lex, c)
#define NNC_LEX_SET(k)          lex->ctok.kind = k
#define NNC_LEX_SET_TERN(k)     (NNC_LEX_SET(k), 0)
#define NNC_LEX_SET_TERNB(k)    NNC_LEX_SET_TERN(k); break
#define NNC_LEX_COMMIT(k)       NNC_LEX_SET(k); break


typedef enum _nnc_tok_kind {
    TOK_AMPERSAND,  TOK_AND,        TOK_ASSIGN,
    TOK_ASTERISK,   TOK_ATPERSAND,  TOK_CBRACE,
    TOK_CBRACKET,   TOK_CHR,        TOK_CIRCUMFLEX,
    TOK_COLON,      TOK_COMMA,      TOK_CPAREN,
    TOK_DOLLAR,     TOK_DOT,        TOK_EOF,
    TOK_EQ,         TOK_EXCMARK,    TOK_GRAVE,
    TOK_GT,         TOK_GTE,        TOK_IDENT,
    TOK_LSHIFT,     TOK_LT,         TOK_LTE,
    TOK_MINUS,      TOK_NEQ,        TOK_NUMBER,
    TOK_OBRACE,     TOK_OBRACKET,   TOK_OPAREN, TOK_OR,
    TOK_PERCENT,    TOK_PLUS,       TOK_QUESTION,
    TOK_QUOTE,      TOK_QUOTES,     TOK_RSHIFT,
    TOK_SEMICOLON,  TOK_SIGN,       TOK_SLASH,
    TOK_STR,        TOK_STRING,     TOK_TILDE,
    TOK_UNDERSCORE, TOK_VLINE,
    // keywords
    TOK_BREAK,      TOK_CASE,       TOK_CAST, 
    TOK_CONTINUE,   TOK_DEFAULT,    TOK_ENUM,
    TOK_ELIF,       TOK_ENT,        TOK_EXT,
    TOK_FOR,        TOK_FN,         TOK_FROM, 
    TOK_F32,        TOK_F64,        TOK_GOTO,
    TOK_IF,         TOK_I8,         TOK_I16,
    TOK_I32,        TOK_I64,        TOK_IMPORT, 
    TOK_NAMESPACE, TOK_PUB,        TOK_RETURN,
    TOK_STRUCT,     TOK_SWITCH,     TOK_SIZEOF,
    TOK_TYPEDEF,    TOK_UNION,      TOK_U8, 
    TOK_U16,        TOK_U32,        TOK_U64,
    TOK_LET,        TOK_LABEL,      TOK_LENGTHOF, 
    TOK_VAR,        TOK_VOID,       TOK_WHILE, 
    TOK_DO,         TOK_ELSE
} nnc_tok_kind;

typedef struct _nnc_tok {
    nnc_u64 size;                   // size of lexeme
    nnc_tok_kind kind;              // token's kind
    char lexeme[NNC_TOK_BUF_MAX];   // lexeme buffer
} nnc_tok;

typedef struct _nnc_lex {
    FILE* fp;               // pointer to FILE retrieved from `fpath`
    char cc, pc;            // current & previous characters
    nnc_ctx cctx;           // current context
    nnc_tok ctok;           // current token
    const char* fpath;      // path to file
} nnc_lex;

nnc_tok_kind nnc_lex_next(nnc_lex* lex);
const char* nnc_tok_str(nnc_tok_kind kind);
void nnc_lex_init(nnc_lex* out_lex, const char* fpath);
void nnc_lex_fini(nnc_lex* lex);
void nnc_lex_keywords_map_fini();

#endif