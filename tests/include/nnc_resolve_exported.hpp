#ifndef _NNC_RESOLVE_EXPORTED_HPP
#define _NNC_RESOLVE_EXPORTED_HPP

#include "../../nnc_test.hpp"

extern "C" nnc_bool nnc_locatable_expr(const nnc_expression* expr);
extern "C" nnc_bool nnc_resolve_int_literal(nnc_int_literal* literal);
extern "C" nnc_bool nnc_resolve_dbl_literal(nnc_dbl_literal* literal);
extern "C" nnc_bool nnc_resolve_chr_literal(nnc_chr_literal* literal);
extern "C" nnc_bool nnc_resolve_str_literal(nnc_str_literal* literal);
extern "C" nnc_bool nnc_resolve_ident(nnc_ident* ident, nnc_st* table);
extern "C" void nnc_resolve_ref_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_not_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_cast_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_plus_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_minus_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_deref_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_sizeof_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_lengthof_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_bitwise_not_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_as_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_dot_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_call_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_scope_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_index_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" nnc_bool nnc_resolve_unary_expr(nnc_unary_expression* unary, nnc_st* table);
extern "C" void nnc_resolve_add_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" void nnc_resolve_mul_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" void nnc_resolve_shift_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" void nnc_resolve_rel_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" void nnc_resolve_bitwise_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" void nnc_resolve_assign_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" void nnc_resolve_comma_expr(nnc_binary_expression* binary, nnc_st* table);
extern "C" nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* binary, nnc_st* table);

#endif