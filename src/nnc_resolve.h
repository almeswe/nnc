#ifndef _NNC_SEMANTIC_H
#define _NNC_SEMANTIC_H

#include "nnc_symtable.h"
#include "nnc_ast_eval.h"
#include "nnc_typecheck.h"

nnc_bool nnc_resolve_expr(nnc_expression* expr, nnc_st* st);

void nnc_resolve_stmt(nnc_statement* stmt, nnc_st* st);
void nnc_resolve(nnc_ast* ast);

#endif