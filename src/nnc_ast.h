#ifndef _NNC_AST_H
#define _NNC_AST_H

#include "nnc_buf.h"
#include "nnc_literal.h"

typedef enum _nnc_expression_kind {
    EXPR_DBL_LITERAL,
    EXPR_INT_LITERAL,
    EXPR_CHR_LITERAL,
    EXPR_STR_LITERAL
} nnc_expression_kind;

typedef struct _nnc_expression {
    nnc_expression_kind kind;
    nnc_heap_ptr exact;
} nnc_expression;

nnc_expression* nnc_expr_new(nnc_expression_kind kind, nnc_heap_ptr exact);

typedef struct _nnc_ast {
    //todo: incomplete
    nnc_expression* expr;
    const char* file;
} nnc_ast;

nnc_ast* nnc_ast_new(const char* file);

#endif