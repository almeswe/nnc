#ifndef _NNC_STATEMENT_H
#define _NNC_STATEMENT_H

#include "nnc_expression.h"

typedef enum _nnc_statement_kind {
    STMT_IF,
    STMT_FOR,
	STMT_EXPR,
	STMT_JUMP,
	STMT_WHILE,
	STMT_BLOCK,
	STMT_EMPTY,
	STMT_SWITCH,
	STMT_IMPORT,
	STMT_LET,
    STMT_NAMESPACE,
	STMT_TYPE_DECL,
    STMT_ENUM_DECL,
	STMT_FUNC_DECL,
	STMT_LABEL_DECL,
    STMT_UNION_DECL,
	STMT_STRUCT_DECL
} nnc_statement_kind;

typedef struct _nnc_statement {
    nnc_statement_kind kind;
    nnc_heap_ptr exact;
} nnc_statement;

typedef struct _nnc_expression_statement {
    nnc_expression* expr;
} _nnc_expression_statement;

typedef struct _nnc_let_statement {
    nnc_ident* var;
    nnc_type* type;
    nnc_expression* init;
} nnc_let_statement;

nnc_let_statement* nnc_let_stmt_new();
nnc_statement* nnc_stmt_new(nnc_statement_kind kind, nnc_heap_ptr exact);

#endif