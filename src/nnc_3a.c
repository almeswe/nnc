#include "nnc_3a.h"

nnc_3a_quad* quads = NULL;
nnc_3a_cgt_cnt cgt_cnt = 0; 

nnc_static void nnc_3a_quads_add(const nnc_3a_quad* quad) {
    buf_add(quads, *quad);
}

nnc_static const nnc_3a_addr* nnc_3a_quads_res() {
    size_t quads_size = buf_len(quads);
    return &quads[quads_size-1].res;
}

nnc_static void nnc_ident_to_3a(const nnc_ident* ident, const nnc_st* st) {
    nnc_3a_quad quad = nnc_3a_quad_new(
        OP_COPY,
        nnc_3a_cgt_new(),
        nnc_3a_name_new(ident)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_unary_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    static const nnc_unary_expression_kind un_op_map[] = {
        [UNARY_MINUS] = OP_MINUS,
    };
    if (unary->expr != NULL) {
        nnc_expr_to_3a(unary->expr, st);
        nnc_3a_addr res = *nnc_3a_quads_res();
        nnc_3a_quad quad = nnc_3a_quad_new(
            un_op_map[unary->kind],
            nnc_3a_cgt_new(),
            res
        );
        nnc_3a_quads_add(&quad);
    }
}

nnc_static void nnc_binary_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    static const nnc_binary_expression_kind bin_op_map[] = {
        [BINARY_ADD]    = OP_ADD,
        [BINARY_SUB]    = OP_SUB,
        [BINARY_MUL]    = OP_MUL,
        [BINARY_DIV]    = OP_DIV,
        [BINARY_MOD]    = OP_MOD, 
        [BINARY_SHR]    = OP_SHR,
        [BINARY_SHL]    = OP_SHL,
        [BINARY_LT]     = OP_LT, 
        [BINARY_GT]     = OP_GT, 
        [BINARY_LTE]    = OP_LTE, 
        [BINARY_GTE]    = OP_GTE,
        [BINARY_EQ]     = OP_EQ,
        [BINARY_NEQ]    = OP_NEQ,
        [BINARY_BW_AND] = OP_BW_AND,
        [BINARY_BW_XOR] = OP_BW_XOR,
        [BINARY_BW_OR]  = OP_BW_OR,
        [BINARY_AND]    = OP_AND,
        [BINARY_OR]     = OP_OR
    };
    nnc_3a_op_kind op = bin_op_map[binary->kind];
    if (binary->kind == BINARY_ASSIGN ||
        binary->kind == BINARY_COMMA) {
        op = OP_COPY;
    }
    nnc_expr_to_3a(binary->lexpr, st);
    nnc_3a_addr lres = *nnc_3a_quads_res();
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr rres = *nnc_3a_quads_res();
    if (binary->kind == BINARY_COMMA) {
        lres = rres;
    }
    nnc_3a_quad quad = nnc_3a_quad_new(
        op,
        nnc_3a_cgt_new(),
        lres, rres
    );
    nnc_3a_quads_add(&quad);
}


void nnc_expr_to_3a(const nnc_expression* expr, const nnc_st* st) {
    switch (expr->kind) {
        case EXPR_IDENT:  nnc_ident_to_3a(expr->exact, st);  break; 
        case EXPR_UNARY:  nnc_unary_to_3a(expr->exact, st);  break;
        case EXPR_BINARY: nnc_binary_to_3a(expr->exact, st); break;
    }
}