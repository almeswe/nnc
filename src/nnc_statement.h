#ifndef _NNC_STATEMENT_H
#define _NNC_STATEMENT_H

#include "nnc_expression.h"

typedef enum _nnc_statement_kind {
    STMT_IF,
	STMT_DO,
    STMT_FOR,
	STMT_EXPR,
	STMT_WHILE,
	STMT_EMPTY,
	STMT_SWITCH,
	STMT_RETURN,
	STMT_IMPORT,
	STMT_LET,
	STMT_TYPE,
	STMT_GOTO,
	STMT_BREAK,
	STMT_CONTINUE,
	STMT_COMPOUND,
	STMT_NAMESPACE,
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

typedef struct _nnc_let_statement {
    nnc_ident* var;
    nnc_type* type;
    nnc_expression* init;
} nnc_let_statement;

typedef struct _nnc_type_statement {
	nnc_type* type;
	nnc_type* as;
} nnc_type_statement;

typedef struct _nnc_expression_statement {
    nnc_expression* expr;
} nnc_expression_statement;

#define NNC_GET_SYMTABLE(stmt) ((nnc_compound_statement*)stmt->body->exact)->scope;

typedef struct _nnc_compound_statement {
	nnc_statement** stmts;
	struct _nnc_st* scope;
} nnc_compound_statement;

typedef struct _nnc_scoped_body {
	nnc_statement* stmt;
	struct _nnc_st* scope;
} nnc_scoped_body;

typedef struct _nnc_cond_n_body {
	nnc_expression* cond;
	nnc_statement* body;
} nnc_cond_n_body;

typedef nnc_cond_n_body nnc_if_branch;  
typedef nnc_cond_n_body nnc_elif_branch;
typedef nnc_statement   nnc_else_branch;

typedef struct _nnc_if_stmt {
	nnc_if_branch* if_br;
	nnc_elif_branch** elif_brs;
	nnc_else_branch* else_br;
} nnc_if_stmt;

typedef nnc_cond_n_body nnc_while_stmt;
typedef nnc_cond_n_body nnc_do_while_stmt;

typedef struct _nnc_for_stmt {
	nnc_statement* init;
	nnc_statement* cond;
	nnc_statement* step;
	nnc_statement* body;
} nnc_for_stmt;

typedef struct _nnc_jump_statement {
	nnc_statement* body;
} nnc_jump_statement;

typedef nnc_jump_statement nnc_return_statement;
typedef nnc_jump_statement nnc_break_statement;
typedef nnc_jump_statement nnc_continue_statement;
typedef nnc_jump_statement nnc_goto_statement;

typedef nnc_statement nnc_top_statement;

typedef struct _nnc_namespace_statement {
	nnc_ident* var;
	nnc_top_statement** stmts;
} nnc_namespace_statement;

typedef nnc_struct_member nnc_var_type;
typedef nnc_struct_member nnc_fn_param;
typedef nnc_struct_member nnc_union_member;

typedef struct _nnc_fn_statement {
	nnc_ident* var;
	nnc_fn_param** params;
	nnc_type* ret;
	nnc_statement* body;
} nnc_fn_statement;

nnc_statement* nnc_stmt_new(nnc_statement_kind kind, nnc_heap_ptr exact);

#endif