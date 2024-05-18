#ifndef __NNC_AST_PARSE_H__
#define __NNC_AST_PARSE_H__

#include "nnc_ast.h"
#include "nnc_lex.h"
#include "nnc_resolve.h"
#include "nnc_typecheck.h"

typedef struct _nnc_parser {
    nnc_lex lex;
    struct _nnc_parser_current {
        nnc_tok tok;
        nnc_ctx ctx;
        nnc_ctx ctx_copy;
    } current;
    struct _nnc_parser_lookup {
        nnc_tok tok;
        nnc_ctx ctx;
        nnc_bool is_first;
    } lookup;
    nnc_st* st;
} nnc_parser;

void nnc_parser_init(
    nnc_parser* out_parser,
    const char* file
);

void nnc_parser_init_with_fp(
    nnc_parser* out_parser,
    FILE* fp
);

void nnc_parser_fini(
    nnc_parser* parser
);

nnc_tok* nnc_parser_get(
    nnc_parser* parser
);

nnc_ctx* nnc_parser_get_ctx(
    nnc_parser* parser
);

nnc_tok* nnc_parser_get_lookup(
    nnc_parser* parser
);

nnc_tok_kind nnc_parser_peek(
    nnc_parser* parser
);

nnc_tok_kind nnc_parser_peek_lookup(
    nnc_parser* parser
);

nnc_tok_kind nnc_parser_next(
    nnc_parser* parser
);

void nnc_parser_undo(
    nnc_parser* parser
);

nnc_tok_kind nnc_parser_expect(
    nnc_parser* parser,
    nnc_tok_kind kind
);

nnc_expression* nnc_parse_expr_reduced(
    nnc_parser* parser
);

nnc_expression* nnc_parse_expr(
    nnc_parser* parser
);

nnc_statement* nnc_parse_stmt(
    nnc_parser* parser
);

nnc_ast* nnc_parse(
    const char* file
);

#endif