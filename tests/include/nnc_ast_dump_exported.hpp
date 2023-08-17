#ifndef _NNC_AST_DUMP_EXPORTED_HPP
#define _NNC_AST_DUMP_EXPORTED_HPP

#include "nnc_ast.h"
#include "nnc_misc.h"

#define DUMP_DATA(i, e) (nnc_dump_data) { .indent = i, .exact = (nnc_heap_ptr)(e) }

typedef struct _nnc_dump_data {
    nnc_i64 indent;
    nnc_heap_ptr exact;
} nnc_dump_data;

extern "C" {
    extern void nnc_dump_expr(nnc_dump_data data);
    extern void nnc_dump_stmt(nnc_dump_data data);
    extern void nnc_dump_compound_stmt(nnc_dump_data data);
    extern nnc_bool nnc_is_escape(nnc_byte code);
    extern nnc_byte nnc_escape(nnc_byte code);
    extern void nnc_dump_indent(nnc_i64 indent);
    extern void nnc_dump_type(const nnc_type* type);
    extern void nnc_dump_chr(nnc_dump_data data);
    extern void nnc_dump_str(nnc_dump_data data);
    extern void nnc_dump_ident(nnc_dump_data data);
    extern void nnc_dump_int(nnc_dump_data data);
    extern void nnc_dump_dbl(nnc_dump_data data);
    extern void nnc_dump_unary(nnc_dump_data data);
    extern void nnc_dump_binary(nnc_dump_data data);
    extern void nnc_dump_ternary(nnc_dump_data data);
    extern void nnc_dump_if_stmt(nnc_dump_data data);
    extern void nnc_dump_do_stmt(nnc_dump_data data);
    extern void nnc_dump_for_stmt(nnc_dump_data data);
    extern void nnc_dump_let_stmt(nnc_dump_data data);
    extern void nnc_dump_goto_stmt(nnc_dump_data data);
    extern void nnc_dump_type_stmt(nnc_dump_data data);
    extern void nnc_dump_while_stmt(nnc_dump_data data);
    extern void nnc_dump_empty_stmt(nnc_dump_data data);
    extern void nnc_dump_break_stmt(nnc_dump_data data);
    extern void nnc_dump_return_stmt(nnc_dump_data data);
    extern void nnc_dump_expr_stmt(nnc_dump_data data);
    extern void nnc_dump_continue_stmt(nnc_dump_data data);
    extern void nnc_dump_namespace_stmt(nnc_dump_data data);
    extern void nnc_dump_fn_stmt(nnc_dump_data data);
}

#endif