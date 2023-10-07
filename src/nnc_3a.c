#include "nnc_3a.h"

nnc_3a_quad_set* sets = NULL;
nnc_3a_cgt_cnt cgt_cnt = 0; 

nnc_static nnc_3a_quad* quads = NULL;

static const nnc_3a_op_kind un_op_map[] = {
    [UNARY_NOT]         = OP_NOT,
    [UNARY_PLUS]        = OP_PLUS,
    [UNARY_MINUS]       = OP_MINUS,
    [UNARY_POSTFIX_DOT] = OP_DOT,
    [UNARY_BITWISE_NOT] = OP_BW_NOT
};

static const nnc_3a_op_kind bin_op_map[] = {
    [BINARY_EQ]     = OP_EQ,
    [BINARY_OR]     = OP_OR,
    [BINARY_LT]     = OP_LT, 
    [BINARY_GT]     = OP_GT, 
    [BINARY_ADD]    = OP_ADD,
    [BINARY_SUB]    = OP_SUB,
    [BINARY_MUL]    = OP_MUL,
    [BINARY_DIV]    = OP_DIV,
    [BINARY_MOD]    = OP_MOD, 
    [BINARY_SHR]    = OP_SHR,
    [BINARY_SHL]    = OP_SHL,
    [BINARY_LTE]    = OP_LTE, 
    [BINARY_GTE]    = OP_GTE,
    [BINARY_NEQ]    = OP_NEQ,
    [BINARY_AND]    = OP_AND,
    [BINARY_BW_OR]  = OP_BW_OR,
    [BINARY_BW_AND] = OP_BW_AND,
    [BINARY_BW_XOR] = OP_BW_XOR,
};

nnc_static void nnc_3a_quads_add(const nnc_3a_quad* quad) {
    buf_add(quads, *quad);
}

nnc_static const nnc_3a_addr* nnc_3a_quads_res() {
    size_t quads_size = buf_len(quads);
    assert(quads_size > 0);
    return &quads[quads_size-1].res;
}

nnc_static void nnc_fconst_to_3a(const nnc_dbl_literal* fconst, const nnc_st* st) {
    nnc_3a_addr arg = nnc_3a_mkf1(fconst);
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_iconst_to_3a(const nnc_int_literal* iconst, const nnc_st* st) {
    nnc_3a_addr arg = nnc_3a_mki1(iconst);
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_ident_to_3a(const nnc_ident* ident, const nnc_st* st) {
    nnc_3a_addr arg = nnc_3a_mkname1(ident);
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_call_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_3a_quad call_quad = {0};
    const nnc_ident* fn = unary->expr->exact;
    const struct _nnc_unary_postfix_call* call = &unary->exact.call;
    for (nnc_u64 i = 0; i < call->argc; i++) {
        nnc_3a_quad arg_quad = { .op = OP_ARG };
        nnc_expr_to_3a(call->args[i], st);
        arg_quad.arg1 = *nnc_3a_quads_res();
        nnc_3a_quads_add(&arg_quad);
    }
    if (unary->type->kind == T_VOID) {
        call_quad = nnc_3a_mkquad(OP_PCALL, {0},
            nnc_3a_mkname1(fn),
            nnc_3a_mki3(call->argc)
        );
    }
    else {
        call_quad = nnc_3a_mkquad(OP_FCALL,
            nnc_3a_mkcgt(),
            nnc_3a_mkname1(fn),
            nnc_3a_mki3(call->argc)
        );
    }
    nnc_3a_quads_add(&call_quad);
}

typedef struct _nnc_dim_data {
    nnc_u32 dims;
    nnc_u32* sizes;
} nnc_dim_data;

nnc_static void nnc_index_to_3a_ex(const nnc_unary_expression* unary, const nnc_st* st, nnc_bool first, nnc_dim_data data) {
    // previous index is stored here
    // in case of first (means right-most) index
    // there are no base so no need to retrieve it
    nnc_3a_addr base = {0};
    if (!first) {
        base = *nnc_3a_quads_res();
    }
    // translating index expression
    nnc_expr_to_3a(unary->exact.index.expr, st);
    nnc_3a_addr index = *nnc_3a_quads_res();
    // then multiply it by the size of type
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_MUL, nnc_3a_mkcgt(), index, 
        nnc_3a_mki2(data.sizes[data.dims-1], &u32_type)
    );
    nnc_3a_quads_add(&quad);
    // again, if this is not right-most index
    // sum it with previous one, and store the sum.
    if (!first) {
        index = *nnc_3a_quads_res();
        quad = nnc_3a_mkquad(
            OP_ADD, nnc_3a_mkcgt(),
            base, index 
        );
        nnc_3a_quads_add(&quad);
    }
    data.dims--;
    // if this is not last index in a sequence
    // calculate next descendent index.
    const nnc_unary_expression* expr = unary->expr->exact;
    if (unary->expr->kind == EXPR_UNARY &&
        expr->kind == UNARY_POSTFIX_INDEX) {
        // may be function call, cast, etc.
        nnc_index_to_3a_ex(unary->expr->exact, st, false, data);
    }
    // otherwise make OP_INDEX quad from variable name
    // and accumulated index value.
    else {
        nnc_3a_addr index = *nnc_3a_quads_res();
        nnc_expr_to_3a(unary->expr, st);
        nnc_3a_addr address = *nnc_3a_quads_res();
        //const nnc_ident* x = unary->expr->exact;
        nnc_3a_quad quad = nnc_3a_mkquad(
            OP_INDEX, nnc_3a_mkcgt(), 
            address, index
        );
        nnc_3a_quads_add(&quad);
    }
}

nnc_static void nnc_index_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_dim_data data = {0};
    const nnc_unary_expression* expr = unary;
    while (expr->kind == UNARY_POSTFIX_INDEX) {
        data.dims++;
        buf_add(data.sizes, expr->type->size);
        expr = expr->expr->exact;
    }
    nnc_index_to_3a_ex(unary, st, true, data);
    buf_free(data.sizes);
}

nnc_static void nnc_sizeof_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_u64 size = unary->exact.size.of->type->size;
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), 
        nnc_3a_mki2(size, unary->type)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lengthof_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr); 
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), 
        nnc_3a_mki2(t_expr->size, unary->type)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_unary_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    switch (unary->kind) {
        case UNARY_SIZEOF:        nnc_sizeof_to_3a(unary, st);   break;
        case UNARY_LENGTHOF:      nnc_lengthof_to_3a(unary, st); break;
        case UNARY_POSTFIX_CALL:  nnc_call_to_3a(unary, st);     break;
        case UNARY_POSTFIX_INDEX: nnc_index_to_3a(unary, st);    break;
        default: {
            nnc_3a_addr arg = {0};
            if (unary->expr != NULL) {
                nnc_expr_to_3a(unary->expr, st);
                arg = *nnc_3a_quads_res();
            }
            nnc_3a_quad quad = nnc_3a_mkquad(
                un_op_map[unary->kind], nnc_3a_mkcgt(), arg
            );
            nnc_3a_quads_add(&quad);
        }
    }
}

nnc_static void nnc_binary_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_3a_addr arg1 = {0};
    nnc_3a_addr arg2 = {0};
    nnc_3a_op_kind op = OP_NONE;
    nnc_expr_to_3a(binary->lexpr, st);
    arg1 = *nnc_3a_quads_res();
    nnc_expr_to_3a(binary->rexpr, st);
    arg2 = *nnc_3a_quads_res();

    switch (binary->kind) {
        case BINARY_COMMA:
        case BINARY_ASSIGN: {
            op = OP_COPY;
            if (binary->kind == BINARY_COMMA) {
                // right expression is the actual result
                // of comma expression, so swap them 
                arg2 = arg1;
            }
            break;
        }
        default: {
            op = bin_op_map[binary->kind];
        }
    }
    nnc_3a_quad quad = nnc_3a_mkquad(
        op, nnc_3a_mkcgt(), arg1, arg2
    );
    nnc_3a_quads_add(&quad);
}


void nnc_expr_to_3a(const nnc_expression* expr, const nnc_st* st) {
    switch (expr->kind) {
        case EXPR_INT_LITERAL: nnc_iconst_to_3a(expr->exact, st); break;
        case EXPR_DBL_LITERAL: nnc_fconst_to_3a(expr->exact, st); break;
        case EXPR_IDENT:  nnc_ident_to_3a(expr->exact, st);  break; 
        case EXPR_UNARY:  nnc_unary_to_3a(expr->exact, st);  break;
        case EXPR_BINARY: nnc_binary_to_3a(expr->exact, st); break;
    }
}

nnc_static void nnc_fn_stmt_to_3a(const nnc_fn_statement* fn_stmt, const nnc_st* st) {
    quads = NULL;
    nnc_3a_quad_set set = {0};
    set.name = fn_stmt->var->name;
    nnc_stmt_to_3a(fn_stmt->body, st);
    set.quads = quads;
    buf_add(sets, set);
}

nnc_static void nnc_expr_stmt_to_3a(const nnc_expression_statement* expr_stmt, const nnc_st* st) {
    nnc_expr_to_3a(expr_stmt->expr, st);
}

nnc_static void nnc_compound_stmt_to_3a(const nnc_compound_statement* compound_stmt, const nnc_st* st) {
    for (nnc_u64 i = 0; i < buf_len(compound_stmt->stmts); i++) {
        nnc_stmt_to_3a(compound_stmt->stmts[i], compound_stmt->scope);
    }
}

void nnc_stmt_to_3a(const nnc_statement* stmt, const nnc_st* st) {
    switch (stmt->kind) {
        case STMT_FN:       nnc_fn_stmt_to_3a(stmt->exact, st);       break;
        case STMT_EXPR:     nnc_expr_stmt_to_3a(stmt->exact, st);     break;
        case STMT_COMPOUND: nnc_compound_stmt_to_3a(stmt->exact, st); break;
    }
}