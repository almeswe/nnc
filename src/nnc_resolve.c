#include "nnc_resolve.h"
#include "nnc_typecheck.h"
#include "nnc_expression.h"

nnc_static nnc_bool nnc_locatable_expr(const nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_STR_LITERAL: return true;
        case EXPR_INT_LITERAL: return false;
        case EXPR_CHR_LITERAL: return false;
        case EXPR_DBL_LITERAL: return false;
        case EXPR_IDENT: {
            const nnc_ident* ident = expr->exact;
            switch (ident->ctx) {
                case IDENT_DEFAULT:        return true;
                case IDENT_FUNCTION:       return true;
                case IDENT_FUNCTION_PARAM: return true;
                default: return false;
            }
        }
        case EXPR_UNARY: {
            const nnc_unary_expression* unary = expr->exact; 
            switch (unary->kind) {
                case UNARY_POSTFIX_DOT:   return true;
                case UNARY_POSTFIX_INDEX: return true;
                case UNARY_POSTFIX_SCOPE: return nnc_locatable_expr(unary->exact.scope.member);
                default: return false;
            }
        }
        case EXPR_BINARY:  return false;
        case EXPR_TERNARY: return false;
        default: return false;
    }
}

nnc_static nnc_bool nnc_resolve_int_literal(nnc_int_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_I8:  literal->type = &i8_type;  break;
        case SUFFIX_U8:  literal->type = &u8_type;  break;
        case SUFFIX_I16: literal->type = &i16_type; break;
        case SUFFIX_U16: literal->type = &u16_type; break;
        case SUFFIX_I32: literal->type = &i32_type; break;
        case SUFFIX_U32: literal->type = &u32_type; break;
        case SUFFIX_I64: literal->type = &i64_type; break;
        case SUFFIX_U64: literal->type = &u64_type; break;
    }
    return true;
}

nnc_static nnc_bool nnc_resolve_dbl_literal(nnc_dbl_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_F32: literal->type = &f32_type; break;
        case SUFFIX_F64: literal->type = &f64_type; break;
    }
    return true;
}

nnc_static nnc_bool nnc_resolve_chr_literal(nnc_chr_literal* literal) {
    literal->type = &u8_type;
    return true;
}

nnc_static nnc_bool nnc_resolve_str_literal(nnc_str_literal* literal) {
    literal->type = nnc_ptr_type_new(&u8_type);
    return true; 
}

nnc_static nnc_bool nnc_resolve_ident(nnc_ident* ident, nnc_st* table) {
    nnc_symbol* sym = nnc_st_get(table, ident->name);
    if (sym == NULL) {
        nnc_deferred_stack_put(table, DEFERRED_IDENT, ident);
        return false;
    }
    ident->ctx = sym->ctx;
    ident->type = sym->type;
    nnc_deferred_stack_pop(ident);
    return true;
}

nnc_static void nnc_resolve_ref_expr(nnc_unary_expression* unary, nnc_st* table) {
    if (!nnc_locatable_expr(unary->expr)) {
        THROW(NNC_SEMANTIC, "cannot reference non locatable expression.");
    }
    nnc_type* inner = nnc_expr_get_type(unary->expr);
    unary->type = nnc_ptr_type_new(inner);
}

nnc_static void nnc_resolve_not_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = &i8_type;
}

nnc_static void nnc_resolve_cast_expr(nnc_unary_expression* unary, nnc_st* table) {
    // note: this is just placeholder, explicit cast 
    // will be resolved later in process of typechecking.
    unary->type = unary->exact.cast.to;
}

nnc_static void nnc_resolve_plus_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = nnc_expr_get_type(unary->expr);
}

nnc_static void nnc_resolve_minus_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = nnc_expr_get_type(unary->expr);
}

nnc_static void nnc_resolve_deref_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_POINTER && inner->kind != TYPE_ARRAY) {
        THROW(NNC_SEMANTIC, sformat("cannot dereference (non array or pointer) \'%s\' type.", nnc_type_tostr(inner)));
    }
}

nnc_static void nnc_resolve_sizeof_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = &u64_type;
}

nnc_static void nnc_resolve_lengthof_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = &u64_type;
}

nnc_static void nnc_resolve_bitwise_not_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = nnc_expr_get_type(unary->expr);
}

nnc_static void nnc_resolve_as_expr(nnc_unary_expression* unary, nnc_st* table) {
    // note: this is just placeholder, `as` is kind of explicit cast
    // which will be resolved later in process of typechecking.
    unary->type = unary->exact.cast.to;
}

nnc_static void nnc_resolve_dot_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_UNION && inner->kind != TYPE_STRUCT) {
        THROW(NNC_SEMANTIC, sformat("cannot access member of (non union or struct) \'%s\' type.", nnc_type_tostr(inner)));
    }
    nnc_ident* member = unary->exact.dot.member->exact;
    for (nnc_u64 i = 0; i < inner->exact.struct_or_union.memberc; i++) {
        nnc_struct_member* struct_member = inner->exact.struct_or_union.members[i];
        if (nnc_sequal(struct_member->var->name, member->name)) {
            unary->type = struct_member->type;
            member->type = unary->type;
            return;
        }
    }
    THROW(NNC_SEMANTIC, sformat("\'%s\' is not member of \'%s\'.", member->name, nnc_type_tostr(inner)));
}

nnc_static void nnc_resolve_call_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_FUNCTION) {
        THROW(NNC_SEMANTIC, sformat("cannot call (non function) \'%s\' type.", nnc_type_tostr(inner)));
    }
    unary->type = inner->exact.fn.ret;
}

nnc_static void nnc_resolve_scope_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner_type = nnc_expr_get_type(unary->expr);
    if (inner_type->kind != TYPE_NAMESPACE) {
        THROW(NNC_SEMANTIC, sformat("cannot reference (non namespace) \'%s\' type.", nnc_type_tostr(inner_type)));
    }
    nnc_ident* member = unary->exact.scope.member->exact;
    nnc_namespace_statement* np = inner_type->exact.name.space;
    nnc_st* inner_st = NNC_GET_SYMTABLE(np);
    nnc_symbol* sym = nnc_st_get(inner_st, member->name);
    if (sym == NULL) {
        THROW(NNC_SEMANTIC, sformat("\'%s\' is not listed in \'%s\'.", member->name, nnc_type_tostr(inner_type)));
    }
    member->ctx = sym->ctx;
    member->type = sym->type;
    unary->type = member->type;
}

nnc_static void nnc_resolve_index_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_ARRAY && inner->kind != TYPE_POINTER) {
        THROW(NNC_SEMANTIC, sformat("cannot index (non pointer or array) \'%s\' type.", nnc_type_tostr(inner)));
    }
    unary->type = inner->base;
}

nnc_static nnc_bool nnc_resolve_unary_expr(nnc_unary_expression* unary, nnc_st* table) {
    typedef void (unary_expr_resolver)(nnc_unary_expression*, nnc_st*);
    static unary_expr_resolver* resolve[] = {
        [UNARY_REF]           = nnc_resolve_ref_expr,
        [UNARY_NOT]           = nnc_resolve_not_expr,
        [UNARY_CAST]          = nnc_resolve_cast_expr,
        [UNARY_PLUS]          = nnc_resolve_plus_expr,
        [UNARY_MINUS]         = nnc_resolve_minus_expr,
        [UNARY_DEREF]         = nnc_resolve_deref_expr,
        [UNARY_SIZEOF]        = nnc_resolve_sizeof_expr,
        [UNARY_LENGTHOF]      = nnc_resolve_lengthof_expr,
        [UNARY_BITWISE_NOT]   = nnc_resolve_bitwise_not_expr,
        [UNARY_POSTFIX_AS]    = nnc_resolve_as_expr,
        [UNARY_POSTFIX_DOT]   = nnc_resolve_dot_expr,
        [UNARY_POSTFIX_CALL]  = nnc_resolve_call_expr,
        [UNARY_POSTFIX_SCOPE] = nnc_resolve_scope_expr,
        [UNARY_POSTFIX_INDEX] = nnc_resolve_index_expr
    };
    if (unary->expr != NULL) {
        if (!nnc_resolve_expr(unary->expr, table)) {
            nnc_deferred_stack_put(table, DEFERRED_UNARY_EXPR, unary);
            return false;
        }
    }
    assert(unary->kind >= UNARY_REF && unary->kind <= UNARY_POSTFIX_INDEX);
    resolve[unary->kind](unary, table);
    nnc_deferred_stack_pop(unary);
    return true;
}

nnc_static nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* expr, nnc_st* table) {
    switch (expr->kind) {
        //case BINARY_SCOPE: return nnc_resolve_scope_expr(expr, table);
        default: return true; //nnc_abort_no_ctx("nnc_resolve_binary_expr: unknown kind.");
    }
    return false;
}

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity) {
    switch (entity->kind) {
        case DEFERRED_EXPR:         return nnc_resolve_expr(entity->exact, entity->context);
        case DEFERRED_IDENT:        return nnc_resolve_ident(entity->exact, entity->context);
        case DEFERRED_UNARY_EXPR:   return nnc_resolve_unary_expr(entity->exact, entity->context);
        default: nnc_abort_no_ctx("nnc_resolve_entity: kind unknown.");
    }
    return false;
}

nnc_bool nnc_resolve_expr(nnc_expression* expr, nnc_st* table) {
    switch (expr->kind) {
        case EXPR_INT_LITERAL:  return nnc_resolve_int_literal(expr->exact);
        case EXPR_DBL_LITERAL:  return nnc_resolve_dbl_literal(expr->exact);
        case EXPR_CHR_LITERAL:  return nnc_resolve_chr_literal(expr->exact);
        case EXPR_STR_LITERAL:  return nnc_resolve_str_literal(expr->exact);
        case EXPR_IDENT:        return nnc_resolve_ident(expr->exact, table);
        case EXPR_UNARY:        return nnc_resolve_unary_expr(expr->exact, table);
        case EXPR_BINARY:       return nnc_resolve_binary_expr(expr->exact, table);
        default: nnc_abort_no_ctx("nnc_resolve_expr: kind unknown.");
    }
    return false;
}