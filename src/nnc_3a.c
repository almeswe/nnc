#include "nnc_3a.h"

//todo: change data structure for quads to linked list
//todo: reduce code size for peephole optimizer.

nnc_3a_code code = NULL;
nnc_3a_data data = {
    .name = ".data",
    .quads = NULL
};

nnc_3a_cgt_cnt cgt_cnt = 0;
nnc_3a_label_cnt label_cnt = 0; 

typedef struct _nnc_dim_data {
    nnc_u32 dims;
    nnc_u32* sizes;
    const nnc_type* base;
} nnc_dim_data;

typedef struct _nnc_loop_branches {
    nnc_u32 b_loop_out;
    nnc_u32 b_loop_iter_ends;
} nnc_loop_branches;

nnc_static nnc_loop_branches loop_branches = {0};
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

nnc_static nnc_3a_addr nnc_3a_mkcgt() {
    return (nnc_3a_addr) {
        .kind = ADDR_CGT,
        .exact.cgt = ++cgt_cnt
    };
}

nnc_static nnc_3a_addr nnc_3a_mkaddr(nnc_3a_addr resv, const nnc_type* rest) {
    return (nnc_3a_addr) {
        .type = rest,
        .kind = resv.kind, 
        .exact = resv.exact
    };
}

nnc_static nnc_u64 nnc_3a_quads_add(const nnc_3a_quad* quad) {
    buf_add(quads, *quad);
    return buf_len(quads) - 1;
}

nnc_static void nnc_3a_data_quads_add(const nnc_3a_quad* quad) {
    buf_add(data.quads, *quad);
}

nnc_static nnc_loop_branches nnc_set_loop_branches(const nnc_3a_quad* b_loop_out,
                                                   const nnc_3a_quad* b_loop_iter_ends) {
    nnc_loop_branches saved = loop_branches;
    assert(b_loop_out->label != 0);
    assert(b_loop_iter_ends->label != 0);
    loop_branches.b_loop_out = b_loop_out->label;
    loop_branches.b_loop_iter_ends = b_loop_iter_ends->label;
    return saved;
}

nnc_static void nnc_restore_loop_branches(const nnc_loop_branches* branches) {
    loop_branches = *branches;
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
    return NULL;
}

nnc_static void nnc_fconst_to_3a(const nnc_dbl_literal* fconst, const nnc_st* st) {
    nnc_3a_addr arg = nnc_3a_mkf1(fconst);
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), fconst->type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_sconst_hint(const nnc_str_literal* sconst, const nnc_st* st) {
    nnc_3a_quad hint_quad = nnc_3a_mkquad(
        OP_HINT_DECL_STRING, nnc_3a_mks1(sconst) 
    );
    hint_quad.hint = (nnc_heap_ptr)sconst;
    nnc_3a_quads_add(&hint_quad);
}

nnc_static void nnc_sconst_to_3a(const nnc_str_literal* sconst, const nnc_st* st) {
    nnc_sconst_hint(sconst, st);
    nnc_3a_addr arg = nnc_3a_mks1(sconst);
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), sconst->type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_cconst_to_3a(const nnc_chr_literal* cconst, const nnc_st* st) {
    nnc_3a_addr arg = nnc_3a_mki2(cconst->exact, &u8_type);
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), cconst->type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_iconst_to_3a(const nnc_int_literal* iconst, const nnc_st* st) {
    nnc_3a_addr arg = nnc_3a_mki1(iconst);
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), iconst->type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static const char* nnc_mk_nested_name(const nnc_ident* ident, const nnc_st* st) {
    nnc_u64 size = 0;
    const nnc_nesting* current = NULL;
    for (current = ident->nesting; current != NULL; current = current->next) {
        size += current->nest->size;
        // add separator size
        size += 1;
    }
    if (size == 0) {
        return ident->name;
    }
    char* nested_name = nnc_cnew(char, size + 1);
    for (current = ident->nesting; current != NULL; current = current->next) {
        nested_name = strcat(nested_name, current->nest->name);
        nested_name = strcat(nested_name, "_");
    }
    nested_name = strcat(nested_name, ident->name);
    return nested_name;
}

nnc_static void nnc_ident_to_3a(const nnc_ident* ident, const nnc_st* st) {
    const char* name = nnc_mk_nested_name(ident, st);
    nnc_3a_addr arg = nnc_3a_mkname2(name, ident->type);
    arg.exact.name.ictx = ident->ictx;
    if (ident->ictx == IDENT_ENUMERATOR) {
        const nnc_enumerator* enumerator = ident->refs.enumerator;
        assert(!nnc_incomplete_type(enumerator->var->type));
        arg = nnc_3a_mki2(
            enumerator->init_const.u, 
            enumerator->var->type
        );
    }
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), arg.type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lval_expr_to_3a(const nnc_expression* expr, const nnc_st* st);

nnc_static void nnc_lval_ident_to_3a(const nnc_ident* ident, const nnc_st* st) {
    const char* ident_name = nnc_mk_nested_name(ident, st);
    nnc_3a_addr arg = nnc_3a_mkname2(ident_name, ident->type);
    arg.exact.name.ictx = ident->ictx;
    nnc_3a_op_kind op = OP_REF;
    //nnc_3a_op_kind op = nnc_arr_or_ptr_type(ident->type) ? OP_COPY : OP_REF;
    nnc_type* type = ident->type;
    if (op == OP_REF) {
        type = nnc_ptr_type_new(type);
    }
    nnc_3a_quad quad = nnc_3a_mkquad1(
        op, nnc_3a_mkcgt(), type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lval_deref_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_expr_to_3a(unary->expr, st);
    // get the address for dereference
    nnc_3a_addr addr = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), addr.type, addr
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lval_dot_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_u64 offset = 0;
    nnc_lval_expr_to_3a(unary->expr, st);
    nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    const nnc_ident* member = unary->exact.dot.member->exact;
    // if pointer type met, likely this is access of a struct or union
    // member via variable that points to this struct or union. 
    if (nnc_ptr_type(t_expr)) {
        assert(nnc_struct_or_union_type(t_expr->base));
        nnc_3a_addr addr = *nnc_3a_quads_res();
        nnc_3a_quad quad = nnc_3a_mkquad1(
            OP_DEREF, nnc_3a_mkcgt(), addr.type->base, addr
        );
        t_expr = nnc_unalias(t_expr->base);
        nnc_3a_quads_add(&quad);
    }
    //todo: this is true when pack of struct is 1
    //in few other cases this may lead to wrong member mappings
    const struct _nnc_struct_or_union_type* exact = &t_expr->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc && t_expr->kind != T_UNION; i++) {
        const nnc_struct_member* m = exact->members[i];
        if (nnc_strcmp(m->var->name, member->name)) {
            break;
        }
        offset += nnc_sizeof(m->texpr->type);
    }
    nnc_3a_addr addr = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_ADD, nnc_3a_mkcgt(), nnc_ptr_type_new(member->type), addr, nnc_3a_mki3(offset)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lval_index_to_3a_ex(const nnc_unary_expression* unary, const nnc_st* st, nnc_bool first, nnc_dim_data data) {
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
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_MUL, nnc_3a_mkcgt(), &i64_type, index, nnc_3a_mki3(typesize)
    );
    nnc_3a_quads_add(&quad);
    // again, if this is not right-most index
    // sum it with previous one, and store the sum.
    if (!first) {
        index = *nnc_3a_quads_res();
        quad = nnc_3a_mkquad1(
            OP_ADD, nnc_3a_mkcgt(), &i64_type, index, base
        );
        nnc_3a_quads_add(&quad);
    }
    data.dims--;
    // if this is not last index in a sequence
    // calculate next descendent index.
    const nnc_unary_expression* expr = unary->expr->exact;
    if (unary->expr->kind == EXPR_UNARY &&
        expr->kind == UNARY_POSTFIX_INDEX) {
        nnc_lval_index_to_3a_ex(unary->expr->exact, st, false, data);
    }
    else {
        nnc_3a_quad quad = {0};
        nnc_3a_addr index = *nnc_3a_quads_res();
        nnc_expr_to_3a(unary->expr, st);
        nnc_3a_addr address = *nnc_3a_quads_res();
        quad = nnc_3a_mkquad1(
            OP_ADD, nnc_3a_mkcgt(), address.type, index, address
        );
        nnc_3a_quads_add(&quad);
    }
}

nnc_static void nnc_lval_index_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_dim_data data = {0};
    const nnc_unary_expression* expr = unary;
    while (expr->kind == UNARY_POSTFIX_INDEX) {
        data.dims++;
        buf_add(data.sizes, nnc_sizeof(expr->type));
        expr = expr->expr->exact;
    }
    nnc_lval_index_to_3a_ex(unary, st, true, data);
    buf_free(data.sizes);
}

nnc_static void nnc_call_to_3a(const nnc_unary_expression* unary, const nnc_st* st);

nnc_static void nnc_lval_unary_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    switch (unary->kind) {
        case UNARY_DEREF:         nnc_lval_deref_to_3a(unary, st); break;
        case UNARY_POSTFIX_DOT:   nnc_lval_dot_to_3a(unary, st);   break;
        case UNARY_POSTFIX_CALL:  nnc_call_to_3a(unary, st);       break;
        case UNARY_POSTFIX_INDEX: nnc_lval_index_to_3a(unary, st); break;
        default: {
            nnc_abort("nnc_lval_unary_to_3a: unknown kind.\n", &unary->ctx);
        }
    }
}

nnc_static void nnc_lval_expr_to_3a(const nnc_expression* expr, const nnc_st* st) {
    switch (expr->kind) {
        case EXPR_IDENT: nnc_lval_ident_to_3a(expr->exact, st); break;
        case EXPR_UNARY: nnc_lval_unary_to_3a(expr->exact, st); break;
        default: {
            nnc_abort("nnc_lval_expr_to_3a: unknown kind.\n", nnc_expr_get_ctx(expr));
        }
    }
}

nnc_static void nnc_ref_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    if (unary->expr->kind != EXPR_IDENT) {
        nnc_lval_expr_to_3a(unary->expr, st);
    }
    else {
        const nnc_ident* ident = unary->expr->exact;
        nnc_3a_quad quad = nnc_3a_mkquad1(
            OP_REF, nnc_3a_mkcgt(), unary->type, nnc_3a_mkname1(ident)
        );
        nnc_3a_quads_add(&quad);
    }
}

nnc_static void nnc_not_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    nnc_expr_to_3a(unary->expr, st);
    nnc_3a_addr res = nnc_3a_mkcgt();
    nnc_3a_addr cond = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPE, nnc_3a_mki3(b_true.label), cond, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, &i8_type, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, &i8_type, nnc_3a_mki3(1)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_cast_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_expr_to_3a(unary->expr, st);
    nnc_3a_addr res = *nnc_3a_quads_res();
    //todo: change this back?
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_CAST, res, unary->type, res
    );
    //nnc_3a_quad quad = nnc_3a_mkquad1(
    //    OP_CAST, nnc_3a_mkcgt(), unary->type, res
    //);
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_deref_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_lval_deref_to_3a(unary, st);
    nnc_3a_addr res = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_DEREF, nnc_3a_mkcgt(), unary->type, res
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_sizeof_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_type* type = nnc_unalias(unary->exact.size.of->type);
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), unary->type, nnc_3a_mki2(nnc_sizeof(type), unary->type)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_lengthof_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr); 
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), unary->type, nnc_3a_mki2(nnc_sizeof(t_expr), unary->type)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_dot_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_ident* member = unary->exact.dot.member->exact;
    nnc_lval_dot_to_3a(unary, st);
    nnc_3a_addr addr = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_DEREF, nnc_3a_mkcgt(), member->type, addr
    );
    nnc_3a_quads_add(&quad);
}

nnc_static nnc_u64 nnc_call_hint(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_3a_quad hint_quad = { .op = OP_HINT_PREPARE_CALL };
    hint_quad.hint = (nnc_heap_ptr)unary;
    return nnc_3a_quads_add(&hint_quad);
}

nnc_static void nnc_call_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_u64 idx = nnc_call_hint(unary, st);
    const struct _nnc_unary_postfix_call* call = &unary->exact.call;
    for (nnc_u64 i = 0; i < call->argc; i++) {
        nnc_expr_to_3a(call->args[i], st);
        nnc_3a_addr arg = *nnc_3a_quads_res();
        nnc_3a_quad quad = {
            .op = OP_ARG, .res.type = arg.type, .arg1 = arg 
        };
        nnc_3a_quads_add(&quad);
    }
    nnc_expr_to_3a(unary->expr, st);
    nnc_3a_addr arg1 = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_PCALL, {0}, arg1, nnc_3a_mki3(call->argc)
    );
    if (unary->type->kind != T_VOID) {
        quad.op = OP_FCALL;
        quad.res = nnc_3a_mkcgt();
        quad.res.type = unary->type;
        quads[idx].res = quad.res;
    }
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_index_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    nnc_lval_index_to_3a(unary, st);
    nnc_3a_addr addr = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_DEREF, nnc_3a_mkcgt(), addr.type->base, addr
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_unary_to_3a(const nnc_unary_expression* unary, const nnc_st* st) {
    switch (unary->kind) {
        case UNARY_REF:           nnc_ref_to_3a(unary, st);      break;
        case UNARY_NOT:           nnc_not_to_3a(unary, st);      break;
        case UNARY_CAST:          nnc_cast_to_3a(unary, st);     break;
        case UNARY_DEREF:         nnc_deref_to_3a(unary, st);    break;
        case UNARY_SIZEOF:        nnc_sizeof_to_3a(unary, st);   break;
        case UNARY_LENGTHOF:      nnc_lengthof_to_3a(unary, st); break;
        case UNARY_POSTFIX_AS:    nnc_cast_to_3a(unary, st);     break;
        case UNARY_POSTFIX_DOT:   nnc_dot_to_3a(unary, st);      break;
        case UNARY_POSTFIX_CALL:  nnc_call_to_3a(unary, st);     break;
        case UNARY_POSTFIX_INDEX: nnc_index_to_3a(unary, st);    break;
        default: {
            nnc_3a_addr arg = {0};
            if (unary->expr != NULL) {
                nnc_expr_to_3a(unary->expr, st);
                arg = *nnc_3a_quads_res();
            }
            nnc_3a_quad quad = nnc_3a_mkquad1(
                un_op_map[unary->kind], arg, unary->type, arg
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
    quad = nnc_3a_mkquad1(
        OP_COPY, res, &i8_type, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, &i8_type, nnc_3a_mki3(1)
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
    quad = nnc_3a_mkquad1(
        OP_COPY, res, &i8_type, nnc_3a_mki3(1)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, &i8_type, nnc_3a_mki3(0)
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
    nnc_3a_quad quad = nnc_3a_mkquad1(
        op, nnc_3a_mki3(b_true.label), binary->type, laddr, raddr
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_addr res = nnc_3a_mkcgt();
    quad = nnc_3a_mkquad1(
        OP_COPY, res, binary->type, nnc_3a_mki3(0)
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, binary->type, nnc_3a_mki3(1)
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

nnc_static void nnc_add_or_sub_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_expr_to_3a(binary->lexpr, st);
    nnc_3a_addr arg1 = *nnc_3a_quads_res();
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr arg2 = *nnc_3a_quads_res();
    if (nnc_arr_or_ptr_type(arg1.type) &&
        !nnc_arr_or_ptr_type(arg2.type)) {
        // apply pointer arithmetic here
        nnc_3a_quad quad = nnc_3a_mkquad1(
            OP_MUL, arg2, arg2.type, arg2, nnc_3a_mki3(nnc_sizeof(arg1.type->base))
        );
        nnc_3a_quads_add(&quad);
    }
    nnc_3a_quad quad = nnc_3a_mkquad1(
        bin_op_map[binary->kind], arg1, binary->type, arg1, arg2
    );
    nnc_3a_quads_add(&quad);
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

nnc_static void nnc_comma_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_expr_to_3a(binary->lexpr, st);
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr arg = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), arg.type, arg
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_assign_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_lval_expr_to_3a(binary->lexpr, st);
    nnc_3a_addr addr = *nnc_3a_quads_res();
    nnc_expr_to_3a(binary->rexpr, st);
    nnc_3a_addr copy = *nnc_3a_quads_res();
    // actual assignment to the calculated address
    nnc_3a_quad quad = nnc_3a_mkquad1(
        OP_DEREF_COPY, addr, copy.type, copy
    );
    nnc_3a_quads_add(&quad);
    // this is the actual result of the evaluation
    // of the right expression, this is return value of the
    // assignment expression
    quad = nnc_3a_mkquad1(
        OP_COPY, nnc_3a_mkcgt(), binary->type, copy
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_binary_to_3a(const nnc_binary_expression* binary, const nnc_st* st) {
    nnc_3a_addr arg1 = {0};
    nnc_3a_addr arg2 = {0};
    nnc_3a_op_kind op = OP_NONE;
    switch (binary->kind) {
        case BINARY_EQ:     nnc_eq_to_3a(binary, st);         break;
        case BINARY_OR:     nnc_or_to_3a(binary, st);         break;
        case BINARY_LT:     nnc_lt_to_3a(binary, st);         break;
        case BINARY_GT:     nnc_gt_to_3a(binary, st);         break;
        case BINARY_LTE:    nnc_lte_to_3a(binary, st);        break;
        case BINARY_GTE:    nnc_gte_to_3a(binary, st);        break;
        case BINARY_AND:    nnc_and_to_3a(binary, st);        break;
        case BINARY_NEQ:    nnc_neq_to_3a(binary, st);        break;
        case BINARY_COMMA:  nnc_comma_to_3a(binary, st);      break;
        case BINARY_ASSIGN: nnc_assign_to_3a(binary, st);     break;
        case BINARY_ADD:    nnc_add_or_sub_to_3a(binary, st); break;
        case BINARY_SUB:    nnc_add_or_sub_to_3a(binary, st); break;
        default: {
            op = bin_op_map[binary->kind];
            nnc_expr_to_3a(binary->lexpr, st);
            arg1 = *nnc_3a_quads_res();
            nnc_expr_to_3a(binary->rexpr, st);
            arg2 = *nnc_3a_quads_res();
            nnc_3a_quad quad = nnc_3a_mkquad1(
                op, arg1, binary->type, arg1, arg2
            );
            nnc_3a_quads_add(&quad);
        }
    }
}

nnc_static void nnc_ternary_to_3a(const nnc_ternary_expression* ternary, const nnc_st* st) {
    nnc_3a_quad b_true = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    nnc_expr_to_3a(ternary->cexpr, st);
    nnc_3a_addr cond = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPT, nnc_3a_mki3(b_true.label), cond
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_addr res = nnc_3a_mkcgt();
    nnc_expr_to_3a(ternary->rexpr, st);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, ternary->type, *nnc_3a_quads_res()
    );
    nnc_3a_quads_add(&quad);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_next.label)
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_true);
    nnc_expr_to_3a(ternary->lexpr, st);
    quad = nnc_3a_mkquad1(
        OP_COPY, res, ternary->type, *nnc_3a_quads_res()
    );
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

void nnc_expr_to_3a(const nnc_expression* expr, const nnc_st* st) {
    switch (expr->kind) {
        case EXPR_IDENT:       nnc_ident_to_3a(expr->exact, st);   break; 
        case EXPR_UNARY:       nnc_unary_to_3a(expr->exact, st);   break;
        case EXPR_BINARY:      nnc_binary_to_3a(expr->exact, st);  break;
        case EXPR_TERNARY:     nnc_ternary_to_3a(expr->exact, st); break;
        case EXPR_STR_LITERAL: nnc_sconst_to_3a(expr->exact, st);  break;
        case EXPR_CHR_LITERAL: nnc_cconst_to_3a(expr->exact, st);  break;
        case EXPR_INT_LITERAL: nnc_iconst_to_3a(expr->exact, st);  break;
        case EXPR_DBL_LITERAL: nnc_fconst_to_3a(expr->exact, st);  break;
        default: {
            nnc_abort("nnc_expr_to_3a: unknown kind.\n", nnc_expr_get_ctx(expr));
        }
    }
}

nnc_static void nnc_do_stmt_to_3a(const nnc_do_while_statement* do_stmt, const nnc_st* st) {
    nnc_loop_branches branches = {0};
    nnc_3a_quad b_iter = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    branches = nnc_set_loop_branches(&b_next, &b_iter);
    nnc_3a_quads_add(&b_iter);
    nnc_stmt_to_3a(do_stmt->body, st);
    nnc_expr_to_3a(do_stmt->cond, st);
    nnc_3a_addr cond = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPF, nnc_3a_mki3(b_next.label), cond
    );
    nnc_3a_quads_add(&quad);    
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_iter.label)
    );
    nnc_restore_loop_branches(&branches);
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

extern void nnc_3a_make_lr_for_proc(nnc_3a_proc* proc);

nnc_static void nnc_fn_stmt_to_3a(const nnc_fn_statement* fn_stmt, const nnc_st* st) {
    cgt_cnt = 0;
    nnc_3a_proc proc = {
        .name = nnc_mk_nested_name(fn_stmt->var, st),
        .quads = (nnc_stmt_to_3a(fn_stmt->body, st), quads),
        .lr_var = map_init_with(8),
        .lr_cgt = map_init_with(32),
        .quad_pointer = 0,
        .local_stack_offset = 0,
        //.param_stack_offset = 0,
        .params = fn_stmt->params
    };
    proc.stat.initial = buf_len(proc.quads);
    #if _NNC_ENABLE_OPTIMIZATIONS
    unit.quads = nnc_3a_optimize(unit.quads, &unit.stat);
    #endif
    _vec_(nnc_3a_basic) blocks = nnc_3a_get_blocks(&proc);
    proc.cfg = nnc_3a_get_cfg(blocks);
    #if _NNC_ENABLE_OPTIMIZATIONS
    unit.cfg = nnc_3a_cfg_optimize(unit.cfg, &unit.stat);
    #endif
    nnc_3a_make_lr_for_proc(&proc);
    quads = NULL;
    buf_add(code, proc);
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

nnc_static void nnc_let_stmt_hint(const nnc_let_statement* let_stmt, const nnc_st* st) {
    nnc_3a_quad hint_quad = nnc_3a_mkquad(
        OP_HINT_DECL_LOCAL, nnc_3a_mkname1(let_stmt->var)
    );
    if (st->ctx == ST_CTX_GLOBAL ||
        st->ctx == ST_CTX_NAMESPACE) {
        hint_quad.op = OP_HINT_DECL_GLOBAL;  
        hint_quad.hint = (nnc_heap_ptr)let_stmt->init;
    }
    nnc_3a_quads_add(&hint_quad);
}

nnc_static void nnc_let_stmt_to_3a(const nnc_let_statement* let_stmt, const nnc_st* st) {
    nnc_let_stmt_hint(let_stmt, st);
    if (let_stmt->init == NULL) {
        return;
    }
    if (st->ctx == ST_CTX_GLOBAL ||
        st->ctx == ST_CTX_NAMESPACE) {
        return;
    }
    nnc_expr_to_3a(let_stmt->init, st);
    nnc_3a_addr resv = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_COPY, nnc_3a_mkname1(let_stmt->var), resv
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_for_stmt_to_3a(const nnc_for_statement* for_stmt, const nnc_st* st) {
    nnc_loop_branches branches = {0};
    nnc_3a_quad b_iter = nnc_3a_mklabel();
    nnc_3a_quad b_step = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    branches = nnc_set_loop_branches(&b_next, &b_step);
    nnc_stmt_to_3a(for_stmt->init, st);
    nnc_3a_quads_add(&b_iter);
    nnc_stmt_to_3a(for_stmt->cond, st);
    if (for_stmt->cond->kind != STMT_EMPTY) {
        nnc_3a_addr cond = *nnc_3a_quads_res();
        nnc_3a_quad quad = nnc_3a_mkquad(
            OP_CJUMPF, nnc_3a_mki3(b_next.label), cond
        );
        nnc_3a_quads_add(&quad);
    }
    nnc_stmt_to_3a(for_stmt->body, st);
    nnc_3a_quads_add(&b_step);
    nnc_stmt_to_3a(for_stmt->step, st);
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_iter.label)
    );
    nnc_restore_loop_branches(&branches);
    nnc_3a_quads_add(&quad);    
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_expr_stmt_to_3a(const nnc_expression_statement* expr_stmt, const nnc_st* st) {
    nnc_expr_to_3a(expr_stmt->expr, st);
}

nnc_static void nnc_while_stmt_to_3a(const nnc_while_statement* while_stmt, const nnc_st* st) {
    nnc_loop_branches branches = {0};
    nnc_3a_quad b_iter = nnc_3a_mklabel();
    nnc_3a_quad b_next = nnc_3a_mklabel();
    branches = nnc_set_loop_branches(&b_next, &b_iter);
    nnc_3a_quads_add(&b_iter);
    nnc_expr_to_3a(while_stmt->cond, st);
    nnc_3a_addr cond = *nnc_3a_quads_res();
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_CJUMPF, nnc_3a_mki3(b_next.label), cond
    );
    nnc_3a_quads_add(&quad);
    nnc_stmt_to_3a(while_stmt->body, st);
    quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(b_iter.label)
    );
    nnc_restore_loop_branches(&branches);
    nnc_3a_quads_add(&quad);
    nnc_3a_quads_add(&b_next);
}

nnc_static void nnc_break_stmt_to_3a(const nnc_break_statement* break_stmt, const nnc_st* st) {
    assert(loop_branches.b_loop_out != 0);
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(loop_branches.b_loop_out)
    );
    nnc_3a_quads_add(&quad);
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
    nnc_3a_quad quad = {
        .op = op, .res.type = arg.type, .arg1 = arg
    };
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_continue_stmt_to_3a(const nnc_continue_statement* continue_stmt, const nnc_st* st) {
    assert(loop_branches.b_loop_iter_ends != 0);
    nnc_3a_quad quad = nnc_3a_mkquad(
        OP_UJUMP, nnc_3a_mki3(loop_branches.b_loop_iter_ends)
    );
    nnc_3a_quads_add(&quad);
}

nnc_static void nnc_compound_stmt_to_3a(const nnc_compound_statement* compound_stmt, const nnc_st* st) {
    for (nnc_u64 i = 0; i < buf_len(compound_stmt->stmts); i++) {
        nnc_stmt_to_3a(compound_stmt->stmts[i], compound_stmt->scope);
    }
}

nnc_static void nnc_namespace_stmt_to_3a(const nnc_namespace_statement* namespace_stmt, const nnc_st* st) {
    nnc_stmt_to_3a(namespace_stmt->body, st);
}

void nnc_stmt_to_3a(const nnc_statement* stmt, const nnc_st* st) {
    switch (stmt->kind) {
        case STMT_TYPE:      break;
        case STMT_EMPTY:     break;
        case STMT_DO:        nnc_do_stmt_to_3a(stmt->exact, st);        break;
        case STMT_FN:        nnc_fn_stmt_to_3a(stmt->exact, st);        break;
        case STMT_IF:        nnc_if_stmt_to_3a(stmt->exact, st);        break;
        case STMT_LET:       nnc_let_stmt_to_3a(stmt->exact, st);       break;
        case STMT_FOR:       nnc_for_stmt_to_3a(stmt->exact, st);       break;
        case STMT_EXPR:      nnc_expr_stmt_to_3a(stmt->exact, st);      break;
        case STMT_WHILE:     nnc_while_stmt_to_3a(stmt->exact, st);     break;
        case STMT_BREAK:     nnc_break_stmt_to_3a(stmt->exact, st);     break;
        case STMT_RETURN:    nnc_return_stmt_to_3a(stmt->exact, st);    break;
        case STMT_CONTINUE:  nnc_continue_stmt_to_3a(stmt->exact, st);  break;
        case STMT_COMPOUND:  nnc_compound_stmt_to_3a(stmt->exact, st);  break;
        case STMT_NAMESPACE: nnc_namespace_stmt_to_3a(stmt->exact, st); break;
    }
}

void nnc_ast_to_3a(const nnc_ast* ast, const nnc_st* st) {
    #ifdef NNC_SHOW_MEMORY_INFO
        fprintf(stderr, "size of `nnc_3a_quad`: %lu\n", sizeof(nnc_3a_quad));
        fprintf(stderr, "`%s` has %lu quads\n", set.name, buf_len(quads));
    #endif
    for (nnc_u64 i = 0; i < buf_len(ast->root); i++) {
        nnc_stmt_to_3a(ast->root[i], ast->st);
    }
}