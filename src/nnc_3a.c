#include "nnc_3a.h"

nnc_3a_quad_set* sets = NULL;
nnc_3a_cgt_cnt cgt_cnt = 0;
nnc_3a_label_cnt label_cnt = 0; 

nnc_static nnc_3a_quad* quads = NULL;

static const nnc_3a_op_kind un_op_map[] = {
    [UNARY_PLUS]        = OP_PLUS,
    [UNARY_MINUS]       = OP_MINUS,
    [UNARY_BITWISE_NOT] = OP_BW_NOT
};

static const nnc_3a_op_kind bin_op_map[] = {
    [BINARY_ADD]    = OP_ADD,
    [BINARY_SUB]    = OP_SUB,
    [BINARY_MUL]    = OP_MUL,
    [BINARY_DIV]    = OP_DIV,
    [BINARY_MOD]    = OP_MOD, 
    [BINARY_SHR]    = OP_SHR,
    [BINARY_SHL]    = OP_SHL,
    [BINARY_BW_OR]  = OP_BW_OR,
    [BINARY_BW_AND] = OP_BW_AND,
    [BINARY_BW_XOR] = OP_BW_XOR
};

nnc_static void nnc_3a_quads_add(const nnc_3a_quad* quad) {
    buf_add(quads, *quad);
}

nnc_static const nnc_3a_addr* nnc_3a_quads_res() {
    size_t offset = 1;
    size_t quads_size = buf_len(quads);
    while (quads_size >= offset) {
        nnc_3a_quad* quad = &quads[quads_size-offset];
        if (quad->label == 0) {
            return &quad->res;
        }
        offset++;
    }
    nnc_abort_no_ctx("nnc_3a_quads_res: bug detected.\n");
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

nnc_static void nnc_not_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    nnc_expr_to_3a(unary->expr, st);
    nnc_3a_addr cond = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPE, nnc_3a_mki3(b_true.label), cond, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_COPY, cond, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad(
        OP_COPY, cond, nnc_3a_mki3(1)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_sizeof_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_u64 size = unary->exact.size.of->type->size;
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), nnc_3a_mki2(size, unary->type)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lengthof_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr); 
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkcgt(), nnc_3a_mki2(t_expr->size, unary->type)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_dot_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_u64 offset = 0;
    nnc_3a_quad quad = {0};
    nnc_expr_to_3a(unary->expr, st);
    const nnc_ident* member = unary->exact.dot.member->exact;
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_ptr_type(t_expr)) {
        quad = nnc_3a_mkquad(
            OP_REF, nnc_3a_mkcgt(), *nnc_3a_quads_res()
        );
        nnc_3a_quads_add(&quad);
    }
    const struct _nnc_struct_or_union_type* exact = &t_expr->exact.struct_or_union;
    //todo: this is true when pack of struct is 1
    // in few other cases this may lead to wrong member mappings
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        if (t_expr->kind == T_UNION) {
            break;
        }
        const nnc_struct_member* m = exact->members[i];
        if (nnc_sequal(m->var->name, member->name)) {
            break;
        }
        offset += m->texpr->type->size;
    }
    quad = nnc_3a_mkquad(
        OP_ADD, nnc_3a_mkcgt(), *nnc_3a_quads_res(), nnc_3a_mki3(offset)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_DEREF, nnc_3a_mkcgt(), *nnc_3a_quads_res(),
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_call_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_ident* fn = unary->expr->exact;
    const struct _nnc_unary_postfix_call* call = &unary->exact.call;
    for (nnc_u64 i = 0; i < call->argc; i++) {
        nnc_expr_to_3a(call->args[i], st);
        nnc_3a_quad quad = nnc_3a_mkquad(
            OP_ARG, {0}, *nnc_3a_quads_res() 
        );
        nnc_3a_quads_add(&quad);
    }
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_PCALL, {0}, nnc_3a_mkname1(fn), nnc_3a_mki3(call->argc)
    );
    if (unary->type->kind != T_VOID) {
        quad.op = OP_FCALL;
        quad.res = nnc_3a_mkcgt();  
    }
    nnc_3a_quads_add(&quad);
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
    nnc_u32 typesize = data.sizes[data.dims-1];
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_MUL, nnc_3a_mkcgt(), index, nnc_3a_mki3(typesize)
    );
    nnc_3a_quads_add(&quad);
    // again, if this is not right-most index
    // sum it with previous one, and store the sum.
    if (!first) {
        index = *nnc_3a_quads_res();
        quad = nnc_3a_mkquad(
            OP_ADD, nnc_3a_mkcgt(), base, index
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
            OP_INDEX, nnc_3a_mkcgt(), address, index
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

nnc_static void nnc_unary_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    switch (unary->kind) {
        case UNARY_NOT:           nnc_not_to_3a(unary, st);      break;
        case UNARY_SIZEOF:        nnc_sizeof_to_3a(unary, st);   break;
        case UNARY_LENGTHOF:      nnc_lengthof_to_3a(unary, st); break;
        case UNARY_POSTFIX_DOT:   nnc_dot_to_3a(unary, st);      break;
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

nnc_static void nnc_or_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    nnc_expr_to_3a(binary->lexpr, st);
    nnc_3a_addr laddr = *nnc_3a_quads_res(); 
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPT, nnc_3a_mki3(b_true.label), laddr
    );
    nnc_3a_quads_add(&quad);
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr raddr = *nnc_3a_quads_res();
    quad = nnc_3a_mkquad(
        OP_CJUMPT, nnc_3a_mki3(b_true.label), raddr
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_addr res = nnc_3a_mkcgt();
    quad = nnc_3a_mkquad(
        OP_COPY, res, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad(
        OP_COPY, res, nnc_3a_mki3(1)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_and_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    nnc_expr_to_3a(binary->lexpr, st);
    nnc_3a_addr laddr = *nnc_3a_quads_res(); 
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPE, nnc_3a_mki3(b_true.label), laddr, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr raddr = *nnc_3a_quads_res();
    quad = nnc_3a_mkquad(
        OP_CJUMPE, nnc_3a_mki3(b_true.label), raddr, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_addr res = nnc_3a_mkcgt();
    quad = nnc_3a_mkquad(
        OP_COPY, res, nnc_3a_mki3(1)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad(
        OP_COPY, res, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_binary_rel_to_3a(const nnc_binary_expression* binary, nnc_3a_op_kind op, const nnc_st* st) {
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    nnc_expr_to_3a(binary->lexpr, st);
    nnc_3a_addr laddr = *nnc_3a_quads_res(); 
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr raddr = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        op, nnc_3a_mki3(b_true.label), laddr, raddr  
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_addr res = nnc_3a_mkcgt();
    quad = nnc_3a_mkquad(
        OP_COPY, res, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad(
        OP_COPY, res, nnc_3a_mki3(1)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_eq_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_binary_rel_to_3a(binary, OP_CJUMPE, st);
}

nnc_static void nnc_lt_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_binary_rel_to_3a(binary, OP_CJUMPLT, st);
}

nnc_static void nnc_gt_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_binary_rel_to_3a(binary, OP_CJUMPGT, st);
}

nnc_static void nnc_lte_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_binary_rel_to_3a(binary, OP_CJUMPLTE, st);
}

nnc_static void nnc_gte_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_binary_rel_to_3a(binary, OP_CJUMPGTE, st);
}

nnc_static void nnc_neq_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_binary_rel_to_3a(binary, OP_CJUMPNE, st);
}

nnc_static void nnc_binary_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_3a_addr arg1 = {0};
    nnc_3a_addr arg2 = {0};
    nnc_3a_op_kind op = OP_NONE;
    switch (binary->kind) {
        case BINARY_EQ:  nnc_eq_to_3a(binary, st);  break;
        case BINARY_OR:  nnc_or_to_3a(binary, st);  break;
        case BINARY_LT:  nnc_lt_to_3a(binary, st);  break;
        case BINARY_GT:  nnc_gt_to_3a(binary, st);  break;
        case BINARY_LTE: nnc_lte_to_3a(binary, st); break;
        case BINARY_GTE: nnc_gte_to_3a(binary, st); break;
        case BINARY_AND: nnc_and_to_3a(binary, st); break;
        case BINARY_NEQ: nnc_neq_to_3a(binary, st); break;
        default: {
            op = bin_op_map[binary->kind];
            nnc_expr_to_3a(binary->lexpr, st);
            arg1 = *nnc_3a_quads_res();
            nnc_expr_to_3a(binary->rexpr, st);
            arg2 = *nnc_3a_quads_res();
            if (binary->kind == BINARY_COMMA ||
                binary->kind == BINARY_ASSIGN) {
                op = OP_COPY;
                if (binary->kind == BINARY_COMMA) {
                    // right expression is the actual result
                    // of comma expression, so swap them 
                    arg2 = arg1;
                }
            }
            nnc_3a_quad quad = nnc_3a_mkquad(
                op, nnc_3a_mkcgt(), arg1, arg2
            );
            nnc_3a_quads_add(&quad);
        }
    }
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
    cgt_cnt = 0;
    nnc_3a_quad_set set = {0};
    set.name = fn_stmt->var->name;
    nnc_stmt_to_3a(fn_stmt->body, st);
    set.quads = quads;
    buf_add(sets, set);
    printf("size of `nnc_3a_quad`: %lu\n", sizeof(nnc_3a_quad));
    printf("`%s` has %lu quads\n", set.name, buf_len(quads));
}

nnc_static void nnc_if_branch_to_3a(const nnc_cond_n_body* branch, const nnc_3a_quad* b_true, 
                                    const nnc_3a_quad* b_next, const nnc_st* st) {
    nnc_expr_to_3a(branch->cond, st);
    nnc_3a_addr cond = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPF, nnc_3a_mki3(b_next->label), cond,
    );
    nnc_3a_quads_add(&quad);
    nnc_stmt_to_3a(branch->body, st);
    if (b_true->label != b_next->label) {
        quad = nnc_3a_mkquad(
            OP_UJUMP, nnc_3a_mki3(b_true->label)
        );
        nnc_3a_quads_add(&quad);
    }
}

nnc_static void nnc_if_stmt_to_3a(const nnc_if_statement* if_stmt, const nnc_st* st) {
    nnc_u64 branches = buf_len(if_stmt->elif_brs);
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = b_true;
    if (branches != 0 || if_stmt->else_br != NULL) {
        b_next = nnc_3a_mklabel();
    }
    nnc_if_branch_to_3a(if_stmt->if_br, &b_true, &b_next, st);
    for (nnc_u64 i = 0; i < branches; i++) {
        nnc_3a_quads_add(&b_next);
        if (i != branches - 1) {
            b_next = nnc_3a_mklabel();
        }
        else {
            b_next = if_stmt->else_br == NULL ? 
                b_true : nnc_3a_mklabel();
        }
        nnc_if_branch_to_3a(if_stmt->elif_brs[i], &b_true, &b_next, st);
    }
    if (if_stmt->else_br != NULL) {
        nnc_3a_quads_add(&b_next);
        nnc_stmt_to_3a(if_stmt->else_br, st);
    }
    nnc_3a_quads_add(&b_true);
}

nnc_static void nnc_expr_stmt_to_3a(const nnc_expression_statement* expr_stmt, const nnc_st* st) {
    nnc_expr_to_3a(expr_stmt->expr, st);
}

nnc_static void nnc_return_stmt_to_3a(const nnc_return_statement* return_stmt, const nnc_st* st) {
    nnc_3a_op_kind op = OP_RETP;
    if (return_stmt->body->kind != STMT_EMPTY) {
        op = OP_RETF;
        nnc_expr_stmt_to_3a(return_stmt->body->exact, st);
    }
    nnc_3a_addr arg = {0};
    if (op == OP_RETF) {
        arg = *nnc_3a_quads_res();
    } 
    nnc_3a_quad quad = nnc_3a_mkquad(
        op, {0}, arg
    );
    nnc_3a_quads_add(&quad);   
}

nnc_static void nnc_compound_stmt_to_3a(const nnc_compound_statement* compound_stmt, const nnc_st* st) {
    for (nnc_u64 i = 0; i < buf_len(compound_stmt->stmts); i++) {
        nnc_stmt_to_3a(compound_stmt->stmts[i], compound_stmt->scope);
    }
}

void nnc_stmt_to_3a(const nnc_statement* stmt, const nnc_st* st) {
    switch (stmt->kind) {
        case STMT_FN:       nnc_fn_stmt_to_3a(stmt->exact, st);       break;
        case STMT_IF:       nnc_if_stmt_to_3a(stmt->exact, st);       break;
        case STMT_EXPR:     nnc_expr_stmt_to_3a(stmt->exact, st);     break;
        case STMT_RETURN:   nnc_return_stmt_to_3a(stmt->exact, st);   break;
        case STMT_COMPOUND: nnc_compound_stmt_to_3a(stmt->exact, st); break;
    }
}