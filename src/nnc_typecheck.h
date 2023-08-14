#ifndef _NNC_TYPECHECK_H
#define _NNC_TYPECHECK_H

#include "nnc_ast.h"

nnc_type* nnc_expr_infer_type(nnc_expression* expr, nnc_st* table);
nnc_type* nnc_expr_get_type(const nnc_expression* expr);

nnc_bool nnc_can_cast_arith_implicitly(const nnc_type* from, const nnc_type* to);
nnc_bool nnc_can_cast_explicitly(const nnc_type* from, const nnc_type* to);

#endif