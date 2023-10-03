#ifndef _NNC_STATEMENT_H
#define _NNC_STATEMENT_H

#include "nnc_expression.h"

#define NNC_GET_SYMTABLE(stmt) \
	(stmt->body->kind == STMT_COMPOUND) ? \
		(((nnc_compound_statement*)stmt->body->exact)->scope) \
		: NULL;

typedef enum _nnc_statement_kind {
	STMT_FN,
    STMT_IF,
	STMT_DO,
	STMT_LET,
    STMT_FOR,
	STMT_TYPE,
	STMT_GOTO,
	STMT_EXPR,
	STMT_WHILE,
	STMT_EMPTY,
	STMT_LABEL,
	STMT_BREAK,
	STMT_SWITCH,
	STMT_RETURN,
	STMT_IMPORT,
	STMT_CONTINUE,
	STMT_COMPOUND,
	STMT_NAMESPACE,
} nnc_statement_kind;

typedef struct _nnc_statement {
    nnc_statement_kind kind;
    nnc_heap_ptr exact;
} nnc_statement;

typedef struct _nnc_let_statement {
    nnc_ident* var;
    nnc_type_expression* texpr;
    nnc_expression* init;
} nnc_let_statement;

typedef struct _nnc_type_statement {
	nnc_type_expression* texpr;
	nnc_type_expression* texpr_as;
} nnc_type_statement;

typedef struct _nnc_expression_statement {
    nnc_expression* expr;
} nnc_expression_statement;

typedef struct _nnc_compound_statement {
	nnc_statement** stmts;
	struct _nnc_st* scope;
} nnc_compound_statement;

typedef struct _nnc_cond_n_body {
	nnc_expression* cond;
	nnc_statement* body;
} nnc_cond_n_body;

typedef nnc_cond_n_body nnc_if_branch;  
typedef nnc_cond_n_body nnc_elif_branch;
typedef nnc_statement   nnc_else_branch;

typedef struct _nnc_if_statement {
	nnc_if_branch* if_br;
	nnc_elif_branch** elif_brs;
	nnc_else_branch* else_br;
} nnc_if_statement;

typedef nnc_cond_n_body nnc_while_statement;
typedef nnc_cond_n_body nnc_do_while_statement;

typedef struct _nnc_for_statement {
	nnc_statement* init;
	nnc_statement* cond;
	nnc_statement* step;
	nnc_statement* body;
} nnc_for_statement;

typedef struct _nnc_jump_statement {
	nnc_ctx ctx;
	nnc_statement* body;
} nnc_jump_statement;

typedef nnc_jump_statement nnc_return_statement;
typedef nnc_jump_statement nnc_break_statement;
typedef nnc_jump_statement nnc_continue_statement;
typedef nnc_jump_statement nnc_goto_statement;

typedef nnc_statement nnc_top_statement;

typedef struct _nnc_namespace_statement {
	nnc_ident* var;
	nnc_statement* body;
} nnc_namespace_statement;

typedef nnc_struct_member nnc_var_type;
typedef nnc_struct_member nnc_fn_param;
typedef nnc_struct_member nnc_union_member;

typedef struct _nnc_fn_statement {
	nnc_ident* var;
	nnc_fn_param** params;
	nnc_type_expression* ret;
	nnc_statement* body;
} nnc_fn_statement;

nnc_statement* nnc_stmt_new(nnc_statement_kind kind, nnc_heap_ptr exact);

#endif