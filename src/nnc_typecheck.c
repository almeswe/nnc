#include "nnc_typecheck.h"

//todo: contexts ??

nnc_bool nnc_incomplete_type(const nnc_type* type) {
    return type->kind == TYPE_VOID ||
        type->kind == TYPE_UNKNOWN ||
        type->kind == TYPE_INCOMPLETE;
}

nnc_bool nnc_fn_type(const nnc_type* type) {
    return type->kind == TYPE_FUNCTION;
}

nnc_bool nnc_ptr_type(const nnc_type* type) {
    return type->kind == TYPE_POINTER;
}

nnc_bool nnc_arr_type(const nnc_type* type) {
    return type->kind == TYPE_ARRAY;
}

nnc_bool nnc_namespace_type(const nnc_type* type) {
    return type->kind == TYPE_NAMESPACE;
}

nnc_bool nnc_arr_or_ptr_type(const nnc_type* type) {
    return type->kind == TYPE_POINTER || type->kind == TYPE_ARRAY;
}

nnc_bool nnc_struct_or_union_type(const nnc_type* type) {
    return type->kind == TYPE_STRUCT || type->kind == TYPE_UNION;
}

nnc_bool nnc_integral_type(const nnc_type* type) {
    switch (type->kind) {
        case TYPE_PRIMITIVE_I8:
        case TYPE_PRIMITIVE_U8:
        case TYPE_PRIMITIVE_I16:
        case TYPE_PRIMITIVE_U16:
        case TYPE_PRIMITIVE_I32:
        case TYPE_PRIMITIVE_U32:
        case TYPE_PRIMITIVE_I64:
        case TYPE_PRIMITIVE_U64:
            return true;
        default:
            return false;
    } 
}

nnc_bool nnc_numeric_type(const nnc_type* type) {
    return nnc_integral_type(type) ||
        type->kind == TYPE_PRIMITIVE_F32         ||
        type->kind == TYPE_PRIMITIVE_F64;
}

const nnc_i32 nnc_rank(const nnc_type* type) {
    nnc_i32 rank = 0;
    const nnc_type* base = type;
    while (nnc_arr_or_ptr_type(base)) {
        rank++;
        base = base->base;
    }
    return rank;
}

const nnc_type* nnc_base_type(const nnc_type* type) {
    if (nnc_incomplete_type(type)) {
        return type;
    }
    const nnc_type* base = type;
    while (!nnc_incomplete_type(base->base)) {
        base = base->base;
    }
    return base;
}

nnc_bool nnc_can_cast_arith_implicitly(const nnc_type* from, const nnc_type* to) {
    static const nnc_i32 hierarchy[] = {
        [TYPE_PRIMITIVE_I8]  = 0,
        [TYPE_PRIMITIVE_U8]  = 1,
        [TYPE_PRIMITIVE_I16] = 2,
        [TYPE_PRIMITIVE_U16] = 3,
        [TYPE_PRIMITIVE_I32] = 4,
        [TYPE_PRIMITIVE_U32] = 5,
        [TYPE_PRIMITIVE_I64] = 6,
        [TYPE_PRIMITIVE_U64] = 7,
        [TYPE_PRIMITIVE_F32] = 8,
        [TYPE_PRIMITIVE_F64] = 9
    };
    if (!nnc_numeric_type(from) || !nnc_numeric_type(to)) {
        return false;
    }
    return hierarchy[from->kind] <= hierarchy[to->kind];
}

nnc_type* nnc_cast_arith_implicitly(const nnc_type* from, const nnc_type* to) {
    if (!nnc_can_cast_arith_implicitly(from, to)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast \'%s\' to \'%s\'.", nnc_type_tostr(from), nnc_type_tostr(to)));
    }
    return (nnc_type*)to;
}

nnc_bool nnc_same_fn_types(const nnc_type* t1, const nnc_type* t2);

nnc_bool nnc_can_cast_assignment_implicitly(const nnc_type* from, const nnc_type* to) {
    if (nnc_can_cast_arith_implicitly(from, to)) {
        return nnc_cast_arith_implicitly(from, to);
    }
    if (nnc_fn_type(to) && nnc_fn_type(from)) {
        return nnc_same_fn_types(to, from);
    }
    if (nnc_ptr_type(to) && nnc_arr_or_ptr_type(from)) {
        if (nnc_rank(from) != nnc_rank(to)) {
            return false;
        }
        return nnc_can_cast_assignment_implicitly(from->base, to->base);
    }
    return false;
}

nnc_bool nnc_same_fn_types(const nnc_type* t1, const nnc_type* t2) {
    assert(t1->kind == TYPE_FUNCTION);
    assert(t2->kind == TYPE_FUNCTION);
    if (t1->exact.fn.paramc != t2->exact.fn.paramc) {
        return false;
    }
    nnc_type** t1_params = t1->exact.fn.params;
    nnc_type** t2_params = t2->exact.fn.params;
    for (nnc_u64 i = 0; i < t1->exact.fn.paramc; i++) {
        if (!nnc_can_cast_assignment_implicitly(t2_params[i], t1_params[i])) {
            return false;
        }
    }
    return nnc_can_cast_assignment_implicitly(t2->exact.fn.ret, t1->exact.fn.ret);
}

nnc_type* nnc_infer_implicitly(const nnc_type* t1, const nnc_type* t2) {
    if (nnc_can_cast_arith_implicitly(t1, t2)) {
        return nnc_cast_arith_implicitly(t1, t2);
    }
    if (nnc_can_cast_arith_implicitly(t2, t1)) {
        return nnc_cast_arith_implicitly(t2, t1);
    }
    THROW(NNC_SEMANTIC, sformat("implicit cast between \'%s\' and \'%s\' is not possible.", nnc_type_tostr(t1), nnc_type_tostr(t2)));
    return NULL;
}

nnc_type* nnc_infer_relate_implicitly(const nnc_type* from, const nnc_type* to) {
    if (!nnc_infer_implicitly(from, to)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast \'%s\' to \'%s\'.", nnc_type_tostr(from), nnc_type_tostr(to)));
    }
    return &i8_type;
}

nnc_type* nnc_infer_assignment_implicitly(const nnc_type* from, const nnc_type* to) {
    if (!nnc_can_cast_assignment_implicitly(from, to)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast \'%s\' to \'%s\'.", nnc_type_tostr(from), nnc_type_tostr(to)));
    }
    return (nnc_type*)to;
}

nnc_bool nnc_can_cast_explicitly(const nnc_type* from, const nnc_type* to) {
    return true;
}

nnc_type* nnc_cast_explicitly(const nnc_type* from, const nnc_type* to) {
    if (nnc_can_cast_explicitly(from, to)) {
        return (nnc_type*)to;
    }
    THROW(NNC_SEMANTIC, sformat("cannot cast \'%s\' to \'%s\'.", nnc_type_tostr(from), nnc_type_tostr(to)));
    return NULL;
} 

nnc_static nnc_type* nnc_chr_infer_type(nnc_chr_literal* literal) {
    return literal->type = &u8_type;
}

nnc_static nnc_type* nnc_str_infer_type(nnc_str_literal* literal) {
    return literal->type = nnc_ptr_type_new(&u8_type);
}

nnc_static nnc_type* nnc_dbl_infer_type(nnc_dbl_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_F32: return literal->type = &f32_type;
        case SUFFIX_F64: return literal->type = &f64_type;
        default: nnc_abort_no_ctx("nnc_dbl_infer_type: unknown suffix.\n");
    }
    return &unknown_type;
}

nnc_static nnc_type* nnc_int_infer_type(nnc_int_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_I8:  return literal->type = &i8_type;
        case SUFFIX_U8:  return literal->type = &u8_type;
        case SUFFIX_I16: return literal->type = &i16_type;
        case SUFFIX_U16: return literal->type = &u16_type;
        case SUFFIX_I32: return literal->type = &i32_type;
        case SUFFIX_U32: return literal->type = &u32_type;
        case SUFFIX_I64: return literal->type = &i64_type;
        case SUFFIX_U64: return literal->type = &u64_type;
        default: nnc_abort_no_ctx("nnc_int_infer_type: unknown suffix.\n");
    }
    return &unknown_type;
}

nnc_static nnc_type* nnc_ident_infer_type(nnc_ident* ident, nnc_st* table) {
    if (ident->ctx == IDENT_NAMESPACE) {
        return &unknown_type;
    }
    return ident->type;
}

nnc_static nnc_type* nnc_unary_expr_infer_type(nnc_unary_expression* expr, nnc_st* table) {
    const nnc_type* type = nnc_expr_infer_type(expr->expr, table);
    switch (expr->kind) {
        case UNARY_CAST:
        case UNARY_POSTFIX_AS:
            return expr->type = nnc_cast_explicitly(type, expr->exact.cast.to);
        default:
            return &unknown_type;
    }
}

nnc_type* nnc_binary_expr_infer_type(nnc_binary_expression* expr, nnc_st* table) {
    nnc_type* ltype = nnc_expr_infer_type(expr->lexpr, table);
    nnc_type* rtype = nnc_expr_infer_type(expr->rexpr, table);
    switch (expr->kind) {
        case BINARY_COMMA:
            return expr->type = rtype;
        case BINARY_ASSIGN:
            return expr->type = nnc_infer_assignment_implicitly(rtype, ltype);
        case BINARY_ADD: case BINARY_SUB:
        case BINARY_MUL: case BINARY_DIV:
        case BINARY_SHL: case BINARY_SHR:
            return expr->type = nnc_infer_implicitly(ltype, rtype);
        case BINARY_BW_OR:
        case BINARY_BW_AND: case BINARY_BW_XOR:
            return expr->type = nnc_infer_implicitly(ltype, rtype);
        case BINARY_OR:  case BINARY_AND:
        case BINARY_EQ:  case BINARY_NEQ:
        case BINARY_GT:  case BINARY_LT:
        case BINARY_GTE: case BINARY_LTE:
            // return value is not used here
            // just need to be sure that types are the same
            nnc_infer_implicitly(ltype, rtype);
            return expr->type = &i8_type;
        default:
            return &unknown_type;
    }
}

nnc_type* nnc_ternary_expr_infer_type(nnc_ternary_expression* expr, nnc_st* table) {
    nnc_type* ltype = nnc_expr_infer_type(expr->lexpr, table);
    nnc_type* rtype = nnc_expr_infer_type(expr->rexpr, table);
    if (nnc_can_cast_assignment_implicitly(rtype, ltype)) {
        return expr->type = nnc_infer_assignment_implicitly(rtype, ltype);
    }
    return expr->type = nnc_infer_assignment_implicitly(ltype, rtype);
}

nnc_type* nnc_expr_infer_type(nnc_expression* expr, nnc_st* table) {
    const nnc_type* type = nnc_expr_get_type(expr);
    if (type->kind != TYPE_UNKNOWN) {
        return (nnc_type*)type;
    }
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: return nnc_chr_infer_type(expr->exact);
        case EXPR_STR_LITERAL: return nnc_str_infer_type(expr->exact);
        case EXPR_DBL_LITERAL: return nnc_dbl_infer_type(expr->exact);
        case EXPR_INT_LITERAL: return nnc_int_infer_type(expr->exact);
        case EXPR_IDENT:       return nnc_ident_infer_type(expr->exact, table);
        case EXPR_UNARY:       return nnc_unary_expr_infer_type(expr->exact, table);
        case EXPR_BINARY:      return nnc_binary_expr_infer_type(expr->exact, table);
        default: nnc_abort_no_ctx("nnc_expr_infer_type: unknown kind.\n");
    }
    return &unknown_type;
}

nnc_type* nnc_expr_get_type(const nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: return ((nnc_chr_literal*)expr->exact)->type;
        case EXPR_STR_LITERAL: return ((nnc_str_literal*)expr->exact)->type;
        case EXPR_DBL_LITERAL: return ((nnc_dbl_literal*)expr->exact)->type;
        case EXPR_INT_LITERAL: return ((nnc_int_literal*)expr->exact)->type;
        case EXPR_IDENT:       return ((nnc_ident*)expr->exact)->type;
        case EXPR_UNARY:       return ((nnc_unary_expression*)expr->exact)->type;
        case EXPR_BINARY:      return ((nnc_binary_expression*)expr->exact)->type;
        case EXPR_TERNARY:     return ((nnc_ternary_expression*)expr->exact)->type;
        default: nnc_abort_no_ctx("nnc_expr_get_type: unknown kind.\n");
    }
    return &unknown_type;
}