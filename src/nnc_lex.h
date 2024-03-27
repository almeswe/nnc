#ifndef __NNC_LEX_H__
#define __NNC_LEX_H__

#include "nnc_ctx.h"

#include "nnc_map.h"
#include "nnc_try_catch.h"

#define NNC_TOK_BUF_MAX 512

typedef enum _nnc_tok_kind {
    TOK_AMPERSAND,  TOK_AND,        TOK_ASSIGN,
    TOK_ASTERISK,   TOK_ATPERSAND,  TOK_CBRACE,
    TOK_CBRACKET,   TOK_CHR,        TOK_CIRCUMFLEX,
    TOK_COLON,      TOK_COMMA,      TOK_CPAREN,
    TOK_DOLLAR,     TOK_DOT,        TOK_DCOLON, TOK_EOF,
    TOK_EQ,         TOK_EXCMARK,    TOK_GRAVE,
    TOK_GT,         TOK_GTE,        TOK_IDENT,
    TOK_LSHIFT,     TOK_LT,         TOK_LTE,
    TOK_MINUS,      TOK_NEQ,        TOK_NUMBER,
    TOK_OBRACE,     TOK_OBRACKET,   TOK_OPAREN,     TOK_OR,
    TOK_PERCENT,    TOK_PLUS,       TOK_QUESTION,
    TOK_QUOTE,      TOK_QUOTES,     TOK_RSHIFT,
    TOK_SEMICOLON,  TOK_SIGN,       TOK_SLASH,
    TOK_STR,        TOK_TILDE,      TOK_UNDERSCORE, 
    TOK_VLINE,
    // keywords
    TOK_AS,         TOK_BREAK,      TOK_CASE,       TOK_CAST, 
    TOK_CONTINUE,   TOK_DEFAULT,    TOK_ENUM,
    TOK_ELIF,       TOK_ENT,        TOK_EXT,
    TOK_FOR,        TOK_FN,         TOK_FROM, 
    TOK_F32,        TOK_F64,        TOK_GOTO,
    TOK_IF,         TOK_I8,         TOK_I16,
    TOK_I32,        TOK_I64,        TOK_IMPORT, 
    TOK_NAMESPACE,  TOK_PUB,        TOK_RETURN,
    TOK_STRUCT,     TOK_SWITCH,     TOK_SIZEOF,
    TOK_TYPE,       TOK_UNION,      TOK_U8, 
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

/**
 * @brief Grabs next token from sequence of characters.
 *  This function is recovers from error automatically.
 * @param lex Pointer to `nnc_lex` instance.
 * @return Kind of token grabbed. 
 */
nnc_tok_kind nnc_lex_next(
    nnc_lex* lex
);

/**
 * @brief Gets string representation of specified token kind.
 * @param kind Token kind for which string representation will be retrieved.
 * @return String representation of a token kind.
 */
const char* nnc_tok_str(
    nnc_tok_kind kind
);

/**
 * @brief Initializes preallocated instance of `nnc_lex`.
 *  Also initializes hash map of keywords, if it is not already initialized.
 * @param out_lex Pointer to preallocated instance of `nnc_lex`.
 * @param fpath Path to target file for lexical analysis.
 * @throw NNC_LEX_BAD_FILE if file cannot be opened for reading.
 */
void nnc_lex_init(
    nnc_lex* out_lex,
    const char* fpath
);

/**
 * @brief Initializes preallocated instance of `nnc_lex`.
 *  with file pointer instead of file path.
 * @param out_lex Pointer to preallocated instance of `nnc_lex`.
 * @param fp Pointer to non-null instance of `FILE`.
 */
void nnc_lex_init_with_fp(
    nnc_lex* out_lex,
    FILE* fp
);

/**
 * @brief Performs error-recovery, by skipping whole line of characters.
 * @param lex Pointer to `nnc_lex` instance.
 */
void nnc_lex_recover(
    nnc_lex* lex
);

/**
 * @brief Finalizes instance of `nnc_lex`.
 * @param lex Pointer to instance of `nnc_lex` to be finalized.
 */
void nnc_lex_fini(
    nnc_lex* lex
);

/**
 * @brief Finalizes hash map of keywords. 
 */
void nnc_lex_keywords_map_fini();

#endif