#ifndef __NNC_SEMANTIC_H__
#define __NNC_SEMANTIC_H__

#include "nnc_symtable.h"
#include "nnc_ast_eval.h"
#include "nnc_typecheck.h"

/**
 * @brief Resolves expression.
 * @param expr Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_bool nnc_resolve_expr(
    nnc_expression* expr,
    nnc_st* st
);

/**
 * @brief Resolves statement.
 * @param stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
void nnc_resolve_stmt(
    nnc_statement* stmt,
    nnc_st* st
);

/**
 * @brief Resolves abstract syntax tree of the program (semantic analysis).
 * @param ast AST to be resolved.
 */
void nnc_resolve(
    nnc_ast* ast
);

#endif