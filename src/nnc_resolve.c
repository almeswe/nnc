#include "nnc_resolve.h"
#include "nnc_typecheck.h"
#include "nnc_expression.h"

nnc_static nnc_u64 nnc_sizeof(const nnc_type* type) {
    assert(!nnc_incomplete_type(type));
    nnc_type* temp = (nnc_type*)type;
    while (temp->kind == TYPE_ALIAS) {
        temp = temp->base;
    }
    return temp->size;
}

nnc_static nnc_bool nnc_wrapable_expr(const nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_STR_LITERAL: return false;
        case EXPR_INT_LITERAL: return true;
        case EXPR_CHR_LITERAL: return true;
        case EXPR_DBL_LITERAL: return true;
        case EXPR_IDENT: {
            const nnc_ident* ident = expr->exact;
            switch (ident->ctx) {
                case IDENT_ENUMERATOR: return true;
                default: return false;
            }
        }
        case EXPR_UNARY: {
            const nnc_unary_expression* unary = expr->exact; 
            switch (unary->kind) {
                case UNARY_POSTFIX_DOT:   return false;
                case UNARY_POSTFIX_INDEX: return false;
                case UNARY_POSTFIX_SCOPE: return nnc_wrapable_expr(unary->exact.scope.member);
                default: break;
            }
            return nnc_wrapable_expr(unary->expr);
        }
        case EXPR_BINARY: {
            const nnc_binary_expression* binary = expr->exact;
            if (binary->kind == BINARY_ASSIGN) {
                return false;
            }
            return nnc_wrapable_expr(binary->lexpr) &&
                   nnc_wrapable_expr(binary->rexpr);
        }
        case EXPR_TERNARY: {
            const nnc_ternary_expression* ternary = expr->exact;
            return nnc_wrapable_expr(ternary->cexpr) &&
                   nnc_wrapable_expr(ternary->lexpr) &&
                   nnc_wrapable_expr(ternary->rexpr);
        }
        default: return false;
    }
}

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
        case EXPR_BINARY: {
            const nnc_binary_expression* binary = expr->exact;
            switch (binary->kind) {
                case BINARY_ASSIGN: return nnc_locatable_expr(binary->lexpr);
                default: return false;
            }
        }
        case EXPR_TERNARY: return false;
        default: return false;
    }
}

nnc_static nnc_bool nnc_needs_resolve_size(const nnc_type* type) {
    return type->kind == TYPE_ALIAS  ||
           type->kind == TYPE_STRUCT ||
           type->kind == TYPE_UNION  ||
           type->kind == TYPE_ARRAY;
}

nnc_static void nnc_complete_type(nnc_type* type, nnc_st* st) {
    if (type->kind != TYPE_INCOMPLETE) {
        return;
    } 
    nnc_type* st_type = NULL;
    if (type->repr != NULL) {
        st_type = nnc_st_get_type(st, type->repr);
        if (st_type != NULL) {
            *type = *st_type;
            nnc_complete_type(type->base, st);
            return;
        }
    }
    THROW(NNC_SEMANTIC, sformat("incomplete type `%s` met.", type->repr));
}

nnc_static nnc_bool nnc_struct_has_circular_dep(const nnc_str inside, const nnc_type* to, nnc_st* st) {
    assert(to->kind == TYPE_STRUCT || to->kind == TYPE_UNION);
    const struct _nnc_struct_or_union_type* exact = &to->exact.struct_or_union;
    // iterate through each memeber of `to` type,  
    // and perform circular dependency check for each member
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        // complete member's type basic information
        // `nnc_resolve_type` function here may cause
        // stack overflow in some cases
        nnc_complete_type(m->type, st);
        // get pure type in context of an array type
        nnc_type* m_ref = m->type;
        while (m_ref->kind == TYPE_ARRAY) {
            m_ref = m_ref->base;
        }
        // if type is alias, we can compare it's name with `inside` criteria.
        if (m_ref->kind == TYPE_ALIAS) {
            // if same type detected, circular dependency met 
            if (nnc_sequal(inside, m_ref->repr)) {
                return true;
            }
            // check circular dependency for next member,
            // but only if this member is struct or union. 
            if (nnc_struct_or_union_type(m_ref->base)) {
                if (nnc_struct_has_circular_dep(inside, m_ref->base, st)) {
                    return true;
                }
            }
        }
    }
    return false;
}

nnc_static nnc_bool nnc_resolve_type(nnc_type* type, nnc_st* table);
nnc_static nnc_bool nnc_resolve_aliased_struct(const nnc_type* alias, nnc_type* ref_type, nnc_st* st);

nnc_static nnc_bool nnc_resolve_aliased_union(const nnc_type* alias, nnc_type* ref_type, nnc_st* st) {
    return nnc_resolve_aliased_struct(alias, ref_type, st);
}

nnc_static nnc_bool nnc_resolve_aliased_struct(const nnc_type* alias, nnc_type* ref_type, nnc_st* st) {
    const nnc_str a_name = alias->repr;
    struct _nnc_struct_or_union_type* exact = &ref_type->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        // complete member's type basic information
        // `nnc_resolve_type` function here may cause
        // stack overflow in some cases
        nnc_complete_type(m->type, st);
        // then we need to get pure base type of `m->type`,
        // because size of alias and array types will be determined
        // based on `->base` type, we need to be sure that this `->base` type is not the same 
        // as `alias` type. Otherwise it will cause circular dependency => crash the compiler with stack overflow
        nnc_type* m_ref = m->type;
        while (m_ref->kind == TYPE_ALIAS ||
               m_ref->kind == TYPE_ARRAY) {
            m_ref = m_ref->base;
        }
        // if pure type is `TYPE_STRUCT` or `TYPE_UNION`, 
        // check it for circular dependency
        if (nnc_struct_or_union_type(m_ref)) {
            if (nnc_struct_has_circular_dep(a_name, m_ref, st)) {
                THROW(NNC_SEMANTIC, sformat("circular dependency met when expanding node "
                    "inside `%s` at `%s::%s`.", a_name, a_name, m->var->name));
            }
        }
    }
    // this made because union type is resolved
    // by this function too (`nnc_resolve_aliased_struct`), so to resolve type further
    // depending on it's type (struct or union), calling generic `nnc_resolve_type` here.
    return nnc_resolve_type(ref_type, st);
}

nnc_static nnc_bool nnc_resolve_aliased_type(nnc_type* alias, nnc_st* st) {
    nnc_type* ref_type = alias;
    while (ref_type->kind == TYPE_ALIAS) {
        ref_type = ref_type->base;
    }
    switch (ref_type->kind) {
        case TYPE_UNION:  return nnc_resolve_aliased_union(alias, ref_type, st); 
        case TYPE_STRUCT: return nnc_resolve_aliased_struct(alias, ref_type, st);
        default: return false;
    }
}

nnc_static nnc_bool nnc_resolve_enumerator(nnc_enumerator* enumerator, nnc_st* table) {
    if (!nnc_wrapable_expr(enumerator->init)) {
        THROW(NNC_WRONG_ENUMERATOR_INITIALIZER, "enumerator initializer must be constant expression.");
    }
    const nnc_type* init_type = nnc_expr_get_type(enumerator->init);
    if (!nnc_integral_type(init_type)) {
        THROW(NNC_SEMANTIC, "enumerator initializer must be of intergral type.");
    }
    enumerator->init_const.d = nnc_evald(enumerator->init, table);
    nnc_deferred_stack_pop(enumerator);
    return true;
}

nnc_static nnc_bool nnc_resolve_enum(nnc_type* type, nnc_st* table) {
    nnc_bool resolved = true;
    for (nnc_u64 i = 0; i < type->exact.enumeration.memberc; i++) {
        nnc_enumerator* enumerator = type->exact.enumeration.members[i];
        resolved &= nnc_resolve_enumerator(enumerator, table);
    }
    return resolved;
}

nnc_static nnc_bool nnc_resolve_alias(nnc_type* type, nnc_st* table) {
    return nnc_resolve_aliased_type(type, table);
}

nnc_static nnc_bool nnc_resolve_array(nnc_type* type, nnc_st* st) {
    nnc_expression* dim = type->exact.array.dim;
    nnc_resolve_expr(dim, st);
    nnc_resolve_type(type->base, st);
    nnc_type* dim_type = nnc_expr_get_type(dim);
    nnc_complete_type(dim_type, st);
    if (!nnc_wrapable_expr(dim)) {
        THROW(NNC_SEMANTIC, "array dimension value must be constant.");
    }
    if (!nnc_integral_type(dim_type)) {
        THROW(NNC_SEMANTIC, "array dimension must be of integral type.");
    }
    type->size = nnc_evald(dim, st) * nnc_sizeof(type->base);
    return true;
}

nnc_static nnc_bool nnc_resolve_union(nnc_type* type, nnc_st* st) {
    type->size = 0;
    struct _nnc_struct_or_union_type* exact = &type->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        nnc_complete_type(m->type, st);
        if (nnc_needs_resolve_size(m->type)) {
            nnc_resolve_type(m->type, st);
        }
        type->size = nnc_max(type->size, nnc_sizeof(m->type));
    }
    return true;
}

nnc_static nnc_bool nnc_resolve_struct(nnc_type* type, nnc_st* st) {
    type->size = 0;
    struct _nnc_struct_or_union_type* exact = &type->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        nnc_complete_type(m->type, st);
        if (nnc_needs_resolve_size(m->type)) {
            nnc_resolve_type(m->type, st);
        }
        type->size += nnc_sizeof(m->type);
    }
    return true;
}

nnc_static nnc_bool nnc_resolve_pointer(nnc_type* type, nnc_st* table) {
    return nnc_resolve_type(type->base, table);
}

nnc_static nnc_bool nnc_resolve_fn(nnc_type* type, nnc_st* table) {
    nnc_bool is_resolved = true;
    for (nnc_u64 i = 0; i < type->exact.fn.paramc && is_resolved; i++) {
        is_resolved &= nnc_resolve_type(type->exact.fn.params[i], table);
    }
    if (type->exact.fn.ret->kind == TYPE_VOID) {
        return is_resolved;
    }
    return is_resolved && nnc_resolve_type(type->exact.fn.ret, table);
}

nnc_static nnc_bool nnc_resolve_type(nnc_type* type, nnc_st* st) {
    nnc_complete_type(type, st);
    if (nnc_primitive_type(type) || 
        nnc_namespace_type(type)) {
        return true;
    }
    switch (type->kind) {
        case TYPE_ENUM:     return nnc_resolve_enum(type, st);
        case TYPE_ALIAS:    return nnc_resolve_alias(type, st);
        case TYPE_ARRAY:    return nnc_resolve_array(type, st);
        case TYPE_UNION:    return nnc_resolve_union(type, st);
        case TYPE_STRUCT:   return nnc_resolve_struct(type, st);
        case TYPE_POINTER:  return nnc_resolve_pointer(type, st);
        case TYPE_FUNCTION: return nnc_resolve_fn(type, st);
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
    if (ident->ctx == IDENT_ENUMERATOR) {
        ident->type = &i64_type;
        ident->refs = sym->refs;
    }
    nnc_deferred_stack_pop(ident);
    return true;
}

nnc_static void nnc_resolve_ref_expr(nnc_unary_expression* unary, nnc_st* table) {
    if (!nnc_locatable_expr(unary->expr)) {
        THROW(NNC_CANNOT_RESOLVE_REF_EXPR, "cannot reference non locatable expression.");
    }
    nnc_type* inner = nnc_expr_get_type(unary->expr);
    unary->type = nnc_ptr_type_new(inner);
}

nnc_static void nnc_resolve_not_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_integral_type(inner) && !nnc_arr_or_ptr_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_NOT_EXPR, "cannot use logical not for this expression.");
    }
    unary->type = &i8_type;
}

nnc_static void nnc_resolve_cast_expr(nnc_unary_expression* unary, nnc_st* table) {
    //todo: check for explicit cast
    unary->type = unary->exact.cast.to;
}

nnc_static void nnc_resolve_plus_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_numeric_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_PLUS_EXPR, "expression must have numeric type.");
    }
    unary->type = nnc_expr_get_type(unary->expr);
}

nnc_static void nnc_resolve_minus_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_numeric_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_MINUS_EXPR, "expression must have numeric type.");
    }
    unary->type = nnc_expr_get_type(unary->expr);
}

nnc_static void nnc_resolve_deref_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_arr_or_ptr_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_DEREF_EXPR, sformat("cannot dereference (non array or pointer) \'%s\' type.", nnc_type_tostr(inner)));
    }
}

nnc_static void nnc_resolve_sizeof_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = &u64_type;
}

nnc_static void nnc_resolve_lengthof_expr(nnc_unary_expression* unary, nnc_st* table) {
    unary->type = &u64_type;
}

nnc_static void nnc_resolve_bitwise_not_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_integral_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_BW_NOT_EXPR, "expression must have integral type.");
    }
    unary->type = nnc_expr_get_type(unary->expr);
}

nnc_static void nnc_resolve_as_expr(nnc_unary_expression* unary, nnc_st* table) {
    nnc_resolve_type(unary->exact.cast.to, table);
    unary->type = unary->exact.cast.to;
}

nnc_static void nnc_resolve_dot_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_struct_or_union_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_DOT_EXPR, sformat("cannot access member of (non union or struct) \'%s\' type.", nnc_type_tostr(inner)));
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
    THROW(NNC_CANNOT_RESOLVE_DOT_EXPR, sformat("\'%s\' is not member of \'%s\'.", member->name, nnc_type_tostr(inner)));
}

nnc_static void nnc_resolve_call_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_fn_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_CALL_EXPR, sformat("cannot call (non function) \'%s\' type.", nnc_type_tostr(inner)));
    }
    unary->type = inner->exact.fn.ret;
}

nnc_static void nnc_resolve_scope_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner_type = nnc_expr_get_type(unary->expr);
    if (!nnc_namespace_type(inner_type)) {
        THROW(NNC_CANNOT_RESOLVE_SCOPE_EXPR, sformat("cannot reference (non namespace) \'%s\' type.", nnc_type_tostr(inner_type)));
    }
    nnc_ident* member = unary->exact.scope.member->exact;
    nnc_namespace_statement* np = inner_type->exact.name.space;
    nnc_st* inner_st = NNC_GET_SYMTABLE(np);
    nnc_symbol* sym = nnc_st_get_below(inner_st, member->name);
    if (sym == NULL) {
        THROW(NNC_CANNOT_RESOLVE_SCOPE_EXPR, sformat("\'%s\' is not listed in \'%s\'.", member->name, nnc_type_tostr(inner_type)));
    }
    member->ctx = sym->ctx;
    member->type = sym->type;
    unary->type = member->type;
}

nnc_static void nnc_resolve_index_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (!nnc_arr_or_ptr_type(inner)) {
        THROW(NNC_CANNOT_RESOLVE_INDEX_EXPR, sformat("cannot index (non pointer or array) \'%s\' type.", nnc_type_tostr(inner)));
    }
    unary->type = inner->base;
}

nnc_static nnc_bool nnc_resolve_unary_expr(nnc_unary_expression* unary, nnc_st* table) {
    if (unary->expr != NULL) {
        if (!nnc_resolve_expr(unary->expr, table)) {
            nnc_deferred_stack_put(table, DEFERRED_UNARY_EXPR, unary);
            return false;
        }
    }
    switch (unary->kind) {
        case UNARY_REF:           nnc_resolve_ref_expr(unary, table);         break;
        case UNARY_NOT:           nnc_resolve_not_expr(unary, table);         break;
        case UNARY_CAST:          nnc_resolve_cast_expr(unary, table);        break;
        case UNARY_PLUS:          nnc_resolve_plus_expr(unary, table);        break;
        case UNARY_MINUS:         nnc_resolve_minus_expr(unary, table);       break;
        case UNARY_DEREF:         nnc_resolve_deref_expr(unary, table);       break;
        case UNARY_SIZEOF:        nnc_resolve_sizeof_expr(unary, table);      break;
        case UNARY_LENGTHOF:      nnc_resolve_lengthof_expr(unary, table);    break;
        case UNARY_BITWISE_NOT:   nnc_resolve_bitwise_not_expr(unary, table); break;
        case UNARY_POSTFIX_AS:    nnc_resolve_as_expr(unary, table);          break;
        case UNARY_POSTFIX_DOT:   nnc_resolve_dot_expr(unary, table);         break;
        case UNARY_POSTFIX_CALL:  nnc_resolve_call_expr(unary, table);        break;
        case UNARY_POSTFIX_SCOPE: nnc_resolve_scope_expr(unary, table);       break;
        case UNARY_POSTFIX_INDEX: nnc_resolve_index_expr(unary, table);       break;
    }
    nnc_deferred_stack_pop(unary);
    return true;
}

nnc_static void nnc_resolve_add_expr(nnc_binary_expression* binary, nnc_st* table) {
    //todo: pointer arithmetic?
    nnc_type* ltype = nnc_expr_get_type(binary->lexpr);
    nnc_type* rtype = nnc_expr_get_type(binary->rexpr);
    if (nnc_arr_or_ptr_type(ltype)) {
        if (!nnc_integral_type(rtype)) {
            THROW(NNC_CANNOT_RESOLVE_ADD_EXPR, "right expression must have integral type.");
        }
        binary->type = ltype;
        return;
    }
    if (nnc_arr_or_ptr_type(rtype)) {
        if (!nnc_integral_type(ltype)) {
            THROW(NNC_CANNOT_RESOLVE_ADD_EXPR, "left expression must have integral type.");
        }
        binary->type = rtype;
        return;
    }
    if (!nnc_numeric_type(ltype) && !nnc_numeric_type(rtype)) {
        THROW(NNC_CANNOT_RESOLVE_ADD_EXPR, "both expressions must have numeric types.");
    }
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static void nnc_resolve_mul_expr(nnc_binary_expression* binary, nnc_st* table) {
    nnc_type* ltype = nnc_expr_get_type(binary->lexpr);
    nnc_type* rtype = nnc_expr_get_type(binary->rexpr);
    if (binary->kind == BINARY_MOD) {
        if (!nnc_integral_type(ltype) || !nnc_integral_type(rtype)) {
            THROW(NNC_CANNOT_RESOLVE_MUL_EXPR, "both expressions must have integral types.");
        }
    }
    if (!nnc_numeric_type(ltype) && !nnc_numeric_type(rtype)) {
        THROW(NNC_CANNOT_RESOLVE_MUL_EXPR, "both expressions must have numeric types.");
    }
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static void nnc_resolve_shift_expr(nnc_binary_expression* binary, nnc_st* table) {
    nnc_type* ltype = nnc_expr_get_type(binary->lexpr);
    nnc_type* rtype = nnc_expr_get_type(binary->rexpr);
    if (!nnc_integral_type(ltype) || !nnc_integral_type(rtype)) {
        THROW(NNC_CANNOT_RESOLVE_SHIFT_EXPR, "both expressions must have integral types.");
    }
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static void nnc_resolve_rel_expr(nnc_binary_expression* binary, nnc_st* table) {
    //todo: pointer arithmetic?
    nnc_type* ltype = nnc_expr_get_type(binary->lexpr);
    nnc_type* rtype = nnc_expr_get_type(binary->rexpr);
    if (nnc_arr_or_ptr_type(ltype)) {
        if (!nnc_integral_type(rtype)) {
            THROW(NNC_CANNOT_RESOLVE_REL_EXPR, "right expression must have integral type.");
        }
    }
    else if (nnc_arr_or_ptr_type(rtype)) {
        if (!nnc_integral_type(ltype)) {
            THROW(NNC_CANNOT_RESOLVE_REL_EXPR, "left expression must have integral type.");
        }
    }   
    else if (!nnc_numeric_type(ltype) && !nnc_numeric_type(rtype)) {
        THROW(NNC_CANNOT_RESOLVE_REL_EXPR, "both expressions must have numeric types.");
    }
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static void nnc_resolve_bitwise_expr(nnc_binary_expression* binary, nnc_st* table) {
    nnc_type* ltype = nnc_expr_get_type(binary->lexpr);
    nnc_type* rtype = nnc_expr_get_type(binary->rexpr);
    if (!nnc_integral_type(ltype) || !nnc_integral_type(rtype)) {
        THROW(NNC_CANNOT_RESOLVE_BITWISE_EXPR, "both expressions must have integral types.");
    }
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static void nnc_resolve_assign_expr(nnc_binary_expression* binary, nnc_st* table) {
    if (!nnc_locatable_expr(binary->lexpr)) {
        THROW(NNC_CANNOT_ASSIGN_EXPR, "left expression must be locatable.");
    }
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static void nnc_resolve_comma_expr(nnc_binary_expression* binary, nnc_st* table) {
    nnc_binary_expr_infer_type(binary, table);
}

nnc_static nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* binary, nnc_st* table) {
    if (!nnc_resolve_expr(binary->lexpr, table) || 
        !nnc_resolve_expr(binary->rexpr, table)) {
        nnc_deferred_stack_put(table, DEFERRED_BINARY_EXPR, binary);
        return false;
    }
    switch (binary->kind) {
        case BINARY_OR:     nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_LT:     nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_GT:     nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_EQ:     nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_ADD:    nnc_resolve_add_expr(binary, table);     break;
        case BINARY_SUB:    nnc_resolve_add_expr(binary, table);     break;
        case BINARY_MUL:    nnc_resolve_mul_expr(binary, table);     break;
        case BINARY_DIV:    nnc_resolve_mul_expr(binary, table);     break;
        case BINARY_MOD:    nnc_resolve_mul_expr(binary, table);     break;
        case BINARY_SHR:    nnc_resolve_shift_expr(binary, table);   break;
        case BINARY_SHL:    nnc_resolve_shift_expr(binary, table);   break;
        case BINARY_NEQ:    nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_AND:    nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_LTE:    nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_GTE:    nnc_resolve_rel_expr(binary, table);     break;
        case BINARY_COMMA:  nnc_resolve_comma_expr(binary, table);   break;
        case BINARY_BW_OR:  nnc_resolve_bitwise_expr(binary, table); break;
        case BINARY_BW_AND: nnc_resolve_bitwise_expr(binary, table); break;
        case BINARY_BW_XOR: nnc_resolve_bitwise_expr(binary, table); break;
        case BINARY_ASSIGN: nnc_resolve_assign_expr(binary, table);  break;
    } 
    nnc_deferred_stack_pop(binary);
    return true;
}

nnc_static nnc_bool nnc_resolve_ternary_expr(nnc_ternary_expression* ternary, nnc_st* table) {
    nnc_expression* exprs[] = {
        ternary->cexpr, ternary->lexpr, ternary->rexpr
    };
    for (nnc_i32 i = 0; i < 3; i++) {
        if (!nnc_resolve_expr(exprs[i], table)) {
            nnc_deferred_stack_put(table, DEFERRED_TERNARY_EXPR, ternary);
            return false;
        }
    }
    const nnc_type* ctype = nnc_expr_get_type(ternary->cexpr);
    if (!nnc_integral_type(ctype)) {
        THROW(NNC_CANNOT_RESOLVE_TERNARY_EXPR, "type of condition must be of integral type.");
    }
    nnc_ternary_expr_infer_type(ternary, table);
    return true;
}

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity) {
    switch (entity->kind) {
        case DEFERRED_EXPR:         return nnc_resolve_expr(entity->exact, entity->context);
        case DEFERRED_TYPE:         return nnc_resolve_type(entity->exact, entity->context);
        case DEFERRED_IDENT:        return nnc_resolve_ident(entity->exact, entity->context);
        case DEFERRED_UNARY_EXPR:   return nnc_resolve_unary_expr(entity->exact, entity->context);
        case DEFERRED_BINARY_EXPR:  return nnc_resolve_binary_expr(entity->exact, entity->context); 
        case DEFERRED_TERNARY_EXPR: return nnc_resolve_ternary_expr(entity->exact, entity->context);
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
        case EXPR_TERNARY:      return nnc_resolve_ternary_expr(expr->exact, table);
        default: nnc_abort_no_ctx("nnc_resolve_expr: kind unknown.");
    }
    return false;
}

nnc_static void nnc_resolve_params(nnc_fn_param** params, nnc_st* st) {
    //todo: add error-recovery
    for (nnc_u64 i = 0; i < buf_len(params); i++) {
        nnc_resolve_type(params[i]->type, st);
    }
}

nnc_static void nnc_resolve_fn_stmt(nnc_fn_statement* fn_stmt, nnc_st* st) {
    nnc_resolve_params(fn_stmt->params, st);
    nnc_resolve_type(fn_stmt->ret, st);
    nnc_resolve_stmt(fn_stmt->body, st);
}

nnc_static void nnc_resolve_let_stmt(nnc_let_statement* let_stmt, nnc_st* st) {
    nnc_resolve_type(let_stmt->type, st);
    if (let_stmt->init != NULL) {
        nnc_resolve_expr(let_stmt->init, st);
    }
}

nnc_static void nnc_resolve_type_stmt(nnc_type_statement* type_stmt, nnc_st* st) {
    //nnc_resolve_type(type_stmt->type, st);
    nnc_resolve_aliased_type(type_stmt->as, st);
}

nnc_static void nnc_resolve_expr_stmt(nnc_expression_statement* expr_stmt, nnc_st* st) {
    nnc_resolve_expr(expr_stmt->expr, st);
}

nnc_static void nnc_resolve_compound_stmt(nnc_compound_statement* compound_stmt, nnc_st* st) {
    nnc_u64 len = buf_len(compound_stmt->stmts);
    for (nnc_u64 i = 0; i < len; i++) {
        nnc_resolve_stmt(compound_stmt->stmts[i], compound_stmt->scope);
    }
}

nnc_static void nnc_resolve_namespace_stmt(nnc_namespace_statement* namespace_stmt, nnc_st* st) {
    nnc_resolve_stmt(namespace_stmt->body, st);
}

void nnc_resolve_stmt(nnc_statement* stmt, nnc_st* st) {
    switch (stmt->kind) {
        case STMT_FN:        nnc_resolve_fn_stmt(stmt->exact, st);        break;
        case STMT_LET:       nnc_resolve_let_stmt(stmt->exact, st);       break;
        case STMT_TYPE:      nnc_resolve_type_stmt(stmt->exact, st);      break;
        case STMT_EXPR:      nnc_resolve_expr_stmt(stmt->exact, st);      break;
        case STMT_COMPOUND:  nnc_resolve_compound_stmt(stmt->exact, st);  break;
        case STMT_NAMESPACE: nnc_resolve_namespace_stmt(stmt->exact, st); break;
        default: break;
    }
}

void nnc_resolve(nnc_ast* ast) {
    nnc_resolve_stmt(ast->root, ast->st);
}