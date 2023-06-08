#include "nnc_parse.h"

void nnc_parser_init(nnc_parser* out_parser, const char* file) {
    nnc_lex_init(&out_parser->lex, file);
}

void nnc_parser_fini(nnc_parser* parser) {
    if (parser != NULL) {
        nnc_lex_fini(&parser->lex);
    }
}

nnc_tok* nnc_parser_get(nnc_parser* parser) {
    return &parser->lex.ctok;
}

nnc_tok_kind nnc_parser_peek(nnc_parser* parser) {
    return parser->lex.ctok.kind;
}

nnc_tok_kind nnc_parser_next(nnc_parser* parser) {
    return nnc_lex_next(&parser->lex);
}

nnc_expression* nnc_parse_dbl(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_dbl_new(tok->lexeme);
    return nnc_expr_new(EXPR_DBL_LITERAL, exact);
}

nnc_expression* nnc_parse_int(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_int_new(tok->lexeme);
    return nnc_expr_new(EXPR_INT_LITERAL, exact);
}

nnc_expression* nnc_parse_number(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    if (strchr(tok->lexeme, '.') != NULL) {
        return nnc_parse_dbl(parser);
    }
    return nnc_parse_int(parser);
}

nnc_expression* nnc_parse_expr(nnc_parser* parser) {
    switch (nnc_parser_peek(parser)) {
        case TOK_NUMBER:
            return nnc_parse_number(parser);
        default:
            THROW(NNC_UNINPLEMENTED, sformat("nnc_parse_expr -> %s.\n", 
                nnc_tok_str(nnc_parser_peek(parser))));
    }
    return NULL;
}

nnc_ast* nnc_parse(const char* file) {
    nnc_parser parser = { 0 };
    nnc_parser_init(&parser, file);
    nnc_parser_next(&parser);
    nnc_ast* ast = nnc_ast_new(file);
    ast->expr = nnc_parse_expr(&parser);
    nnc_parser_fini(&parser);
    return ast;
}