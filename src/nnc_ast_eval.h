#ifndef _NNC_AST_EVAL_H
#define _NNC_AST_EVAL_H

#include "nnc_literal.h"
#include "nnc_symtable.h"
#include "nnc_expression.h"

//todo: add evaluation for unsigned and float expressions
//todo: add support for determining the type of 
// subexpression and use appropriate function for evaluation
nnc_i64 nnc_evald(const nnc_expression* expr, const nnc_st* st);

#endif