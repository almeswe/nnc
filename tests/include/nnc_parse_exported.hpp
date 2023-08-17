#ifndef _NNC_PARSE_TEST_EXPORTED_HPP
#define _NNC_PARSE_TEST_EXPORTED_HPP

#include "../nnc_test.hpp"

extern "C" {
    extern nnc_bool nnc_parser_match_type(nnc_tok_kind kind);
    extern void nnc_parser_enter_scope(nnc_parser* parser);
    extern void nnc_parser_leave_scope(nnc_parser* parser);
    extern nnc_type* nnc_parse_type(nnc_parser* parser);
    extern nnc_var_type* nnc_parse_var_type(nnc_parser* parser);
    extern nnc_fn_param* nnc_parse_fn_param(nnc_parser* parser);
    extern nnc_struct_member* nnc_parse_struct_member(nnc_parser* parser);
    extern nnc_enumerator* nnc_parse_enumerator(nnc_parser* parser, nnc_type* in_enum);
    extern nnc_type* nnc_parse_fn_type(nnc_parser* parser);
    extern nnc_type* nnc_parse_user_type(nnc_parser* parser);
    extern nnc_type* nnc_parse_enum_type(nnc_parser* parser);
    extern nnc_type* nnc_parse_struct_or_union_type(nnc_parser* parser);
    extern nnc_type* nnc_parse_arr_declarator(nnc_parser* parser, nnc_type* type);
    extern nnc_type* nnc_parse_ptr_declarator(nnc_parser* parser, nnc_type* type);
    extern nnc_type* nnc_parse_type_declarators(nnc_parser* parser, nnc_type* type);
    extern nnc_expression* nnc_parse_dbl(nnc_parser* parser);
    extern nnc_expression* nnc_parse_int(nnc_parser* parser);
    extern nnc_expression* nnc_parse_chr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_str(nnc_parser* parser);
    extern nnc_expression* nnc_parse_ident(nnc_parser* parser);
    extern nnc_expression* nnc_parse_number(nnc_parser* parser);
    extern nnc_expression* nnc_parse_parens(nnc_parser* parser);
    extern nnc_expression* nnc_parse_primary_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_unary_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_cast_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_plus_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_minus_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_bitwise_not_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_dereference_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_reference_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_not_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_sizeof_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_lengthof_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_as_expr(nnc_parser* parser, nnc_expression* prefix);
    extern nnc_expression* nnc_parse_postfix_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_dot_expr(nnc_parser* parser, nnc_expression* prefix);
    extern nnc_expression* nnc_parse_scope_expr(nnc_parser* parser, nnc_expression* prefix);
    extern nnc_expression* nnc_parse_call_expr(nnc_parser* parser, nnc_expression* prefix);
    extern nnc_expression* nnc_parse_index_expr(nnc_parser* parser, nnc_expression* prefix);
    extern nnc_expression* nnc_parse_postfix_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_unary_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_arith_multiplication_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_arith_addition_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_arith_shift_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_relation_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_equality_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_bitwise_and_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_bitwise_xor_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_bitwise_or_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_and_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_or_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_ternary_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_assignment_expr(nnc_parser* parser);
    extern nnc_expression* nnc_parse_comma_expr(nnc_parser* parser);
    extern nnc_statement* nnc_parse_expr_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_body(nnc_parser* parser);
    extern nnc_cond_n_body* nnc_parse_cond_n_body(nnc_parser* parser);
    extern nnc_statement* nnc_parse_if_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_do_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_let_stmt_with_opt_st(nnc_parser* parser, nnc_bool put_in_st);
    extern nnc_statement* nnc_parse_let_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_for_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_goto_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_type_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_while_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_break_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_return_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_topmost_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_namespace_compound_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_compound_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_continue_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_topmost_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_fn_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_namespace_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_topmost_let_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_topmost_type_stmt(nnc_parser* parser);
    extern nnc_statement* nnc_parse_topmost_stmt(nnc_parser* parser);
}

#endif