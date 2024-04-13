#ifndef __NNC_AST_EVAL_H__
#define __NNC_AST_EVAL_H__

#include "nnc_literal.h"
#include "nnc_symtable.h"
#include "nnc_expression.h"

nnc_i64 nnc_evald(
    const nnc_expression* expr,
    const nnc_st* st
);

#endif