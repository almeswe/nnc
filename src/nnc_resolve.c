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
        case EXPR_BINARY: {
            const nnc_binary_expression* binary = expr->exact;
            switch (binary->kind) {
                case BINARY_ASSIGN: return nnc_locatable_expr(binary->lexpr);
                default:            return false;
            }
        }
        case EXPR_TERNARY: return false;
        default:           return false;
    }
}

nnc_static nnc_bool nnc_resolve_alias_type(nnc_type* type, nnc_st* table) {
    assert(type->kind == TYPE_ALIAS);
    nnc_type* from_st = nnc_st_get_type(table, type->repr);
    if (from_st == NULL) {
        return false;
    }
    type->base = from_st;
    return true;
}

nnc_static nnc_bool nnc_resolve_type(nnc_type* type, nnc_st* table) {
    switch (type->kind) {
        case TYPE_ALIAS:         return nnc_resolve_alias_type(type, table);
        case TYPE_PRIMITIVE_I8:  return true;
        case TYPE_PRIMITIVE_U8:  return true;
        case TYPE_PRIMITIVE_I16: return true;
        case TYPE_PRIMITIVE_U16: return true;
        case TYPE_PRIMITIVE_I32: return true;
        case TYPE_PRIMITIVE_U32: return true;
        case TYPE_PRIMITIVE_I64: return true;
        case TYPE_PRIMITIVE_U64: return true;
        case TYPE_PRIMITIVE_F32: return true;
        case TYPE_PRIMITIVE_F64: return true;
        default: return false;
    }
    return false;
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
    if (!nnc_resolve_type(unary->exact.cast.to, table)) {
        //todo: actually put it to deferred_stack (later)
        THROW(NNC_CANNOT_RESOLVE_TYPE, "cannot resolve type.");
    }
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
    if (!nnc_resolve_expr(binary->lexpr, table)) {
        nnc_deferred_stack_put(table, DEFERRED_BINARY_EXPR, binary);
        return false;
    }
    if (!nnc_resolve_expr(binary->rexpr, table)) {
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

nnc_bool nnc_resolve_stmt(nnc_statement* stmt, nnc_st* table) {
    switch (stmt->kind) {
        case STMT_LET:  return nnc_resolve_let_stmt(stmt->exact, table);
        default:        return false;
    }
}