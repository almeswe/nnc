#ifndef _NNC_TYPECHECK_H
#define _NNC_TYPECHECK_H

#include "nnc_ast.h"

nnc_bool nnc_fn_type(const nnc_type* type);
nnc_bool nnc_ptr_type(const nnc_type* type);
nnc_bool nnc_arr_type(const nnc_type* type);
nnc_bool nnc_numeric_type(const nnc_type* type);
nnc_bool nnc_integral_type(const nnc_type* type);
nnc_bool nnc_namespace_type(const nnc_type* type);
nnc_bool nnc_arr_or_ptr_type(const nnc_type* type);
nnc_bool nnc_incomplete_type(const nnc_type* type);
nnc_bool nnc_struct_or_union_type(const nnc_type* type);

nnc_type* nnc_expr_infer_type(nnc_expression* expr, nnc_st* table);
nnc_type* nnc_binary_expr_infer_type(nnc_binary_expression* expr, nnc_st* table);
nnc_type* nnc_ternary_expr_infer_type(nnc_ternary_expression* expr, nnc_st* table);
nnc_type* nnc_expr_get_type(const nnc_expression* expr);

nnc_bool nnc_can_cast_arith_implicitly(const nnc_type* from, const nnc_type* to);
nnc_bool nnc_can_cast_explicitly(const nnc_type* from, const nnc_type* to);

#endif