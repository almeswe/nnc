#ifndef _NNC_AST_PARSE_H
#define _NNC_AST_PARSE_H

#include "nnc_ast.h"
#include "nnc_lex.h"

typedef struct _nnc_parser {
    nnc_lex lex;
} nnc_parser;

void nnc_parser_init(nnc_parser* out_parser, const char* file);
void nnc_parser_fini(nnc_parser* parser);

nnc_tok* nnc_parser_get(nnc_parser* parser);
nnc_tok_kind nnc_parser_peek(nnc_parser* parser);
nnc_tok_kind nnc_parser_next(nnc_parser* parser);
nnc_tok_kind nnc_parser_expect(nnc_parser* parser, nnc_tok_kind kind);

nnc_expression* nnc_parse_expr(nnc_parser* parser);
nnc_ast* nnc_parse(const char* file);

#endif