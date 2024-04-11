#include "nnc_ast_eval.h"
#include "nnc_typecheck.h"

nnc_static nnc_i64 nnc_evald_ident(const nnc_ident* ident, const nnc_st* st) {
    assert(ident->ictx == IDENT_ENUMERATOR);
    if (st != NULL) {
        nnc_sym* sym = nnc_st_get_sym(st, ident->name);
        assert(sym != NULL && sym->ictx == IDENT_ENUMERATOR);
        return sym->refs.enumerator->init_const.d;
    }
    return ident->refs.enumerator->init_const.d;
}

nnc_static nnc_i64 nnc_evald_int_literal(const nnc_int_literal* literal) {
    return literal->exact.d;
}

nnc_static nnc_i64 nnc_evald_chr_literal(const nnc_chr_literal* literal) {
    return (nnc_i64)literal->exact;
}

nnc_static nnc_f64 nnc_evald_dbl_literal(const nnc_dbl_literal* literal) {
    return literal->exact;
}

nnc_static nnc_i64 nnc_evald_cast(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_type* pure = nnc_unalias(unary->exact.cast.to->type);
    switch (pure->kind) {
        case T_ARRAY:         return (nnc_u64)nnc_evald(unary->expr, st);
        case T_POINTER:       return (nnc_u64)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_I8:  return (nnc_i8)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_I16: return (nnc_i16)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_I32: return (nnc_i32)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_I64: return (nnc_i64)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_U8:  return (nnc_u8)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_U16: return (nnc_u16)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_U32: return (nnc_u32)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_U64: return (nnc_u64)nnc_evald(unary->expr, st);
        case T_PRIMITIVE_F32:
        case T_PRIMITIVE_F64: {
            nnc_abort("nnc_evald_cast: float evaluation is not supported.\n", nnc_expr_get_ctx(unary->expr));
        }
    }
    return 0;
}

nnc_static nnc_i64 nnc_evald_sizeof(const nnc_unary_expression* unary, const nnc_st* st) {
    const nnc_type_expression* size_of = unary->exact.size.of;
    return nnc_sizeof(size_of->type);
}

nnc_static nnc_i64 nnc_evald_lengthof(const nnc_unary_expression* unary, const nnc_st* st) {
    return nnc_sizeof(nnc_expr_get_type(unary->expr));
}

nnc_static nnc_i64 nnc_evald_unary(const nnc_unary_expression* unary, const nnc_st* st) {
    switch (unary->kind) {
        //case UNARY_REF:          
        case UNARY_NOT:         return !nnc_evald(unary->expr, st);
        case UNARY_PLUS:        return +nnc_evald(unary->expr, st);
        case UNARY_MINUS:       return -nnc_evald(unary->expr, st);
        case UNARY_BITWISE_NOT: return ~nnc_evald(unary->expr, st);

        case UNARY_CAST:        return nnc_evald_cast(unary, st);
        case UNARY_SIZEOF:      return nnc_evald_sizeof(unary, st);
        case UNARY_LENGTHOF:    return nnc_evald_lengthof(unary, st);
        case UNARY_POSTFIX_AS:  return nnc_evald_cast(unary, st);
        default: {
            nnc_abort("nnc_evald_unary: cannot evaluate this kind of expression.\n", nnc_expr_get_ctx(unary->expr));
        }
    }
    return 0;
}

nnc_static nnc_i64 nnc_evald_binary(const nnc_binary_expression* binary, const nnc_st* st) {
    switch (binary->kind) {
        case BINARY_OR:     return nnc_evald(binary->lexpr, st) || nnc_evald(binary->rexpr, st);
        case BINARY_LT:     return nnc_evald(binary->lexpr, st) < nnc_evald(binary->rexpr, st);
        case BINARY_GT:     return nnc_evald(binary->lexpr, st) > nnc_evald(binary->rexpr, st);
        case BINARY_EQ:     return nnc_evald(binary->lexpr, st) == nnc_evald(binary->rexpr, st);
        case BINARY_ADD:    return nnc_evald(binary->lexpr, st) + nnc_evald(binary->rexpr, st);
        case BINARY_SUB:    return nnc_evald(binary->lexpr, st) - nnc_evald(binary->rexpr, st);
        case BINARY_MUL:    return nnc_evald(binary->lexpr, st) * nnc_evald(binary->rexpr, st);
        case BINARY_DIV:    return nnc_evald(binary->lexpr, st) / nnc_evald(binary->rexpr, st);
        case BINARY_MOD:    return nnc_evald(binary->lexpr, st) % nnc_evald(binary->rexpr, st);
        case BINARY_SHR:    return nnc_evald(binary->lexpr, st) >> nnc_evald(binary->rexpr, st);
        case BINARY_SHL:    return nnc_evald(binary->lexpr, st) << nnc_evald(binary->rexpr, st);
        case BINARY_NEQ:    return nnc_evald(binary->lexpr, st) != nnc_evald(binary->rexpr, st);
        case BINARY_AND:    return nnc_evald(binary->lexpr, st) && nnc_evald(binary->rexpr, st);
        case BINARY_LTE:    return nnc_evald(binary->lexpr, st) <= nnc_evald(binary->rexpr, st);
        case BINARY_GTE:    return nnc_evald(binary->lexpr, st) >= nnc_evald(binary->rexpr, st);
        case BINARY_COMMA:  return nnc_evald(binary->rexpr, st);
        case BINARY_BW_OR:  return nnc_evald(binary->lexpr, st) | nnc_evald(binary->rexpr, st);
        case BINARY_BW_AND: return nnc_evald(binary->lexpr, st) & nnc_evald(binary->rexpr, st);
        case BINARY_BW_XOR: return nnc_evald(binary->lexpr, st) ^ nnc_evald(binary->rexpr, st);
        default: {
            nnc_abort("nnc_evald_binary: cannot evaluate this kind of expression.\n", nnc_expr_get_ctx(binary->lexpr));
        }
    }
    return 0;
}

nnc_static nnc_i64 nnc_evald_ternary(const nnc_ternary_expression* ternary, const nnc_st* st) {
    if (nnc_evald(ternary->cexpr, st)) {
        return nnc_evald(ternary->lexpr, st);
    }
    return nnc_evald(ternary->rexpr, st);
}

nnc_i64 nnc_evald(const nnc_expression* expr, const nnc_st* st) {
    switch (expr->kind) {
        case EXPR_IDENT:       return nnc_evald_ident(expr->exact, st);  
        case EXPR_INT_LITERAL: return nnc_evald_int_literal(expr->exact);
        case EXPR_CHR_LITERAL: return nnc_evald_chr_literal(expr->exact);
        case EXPR_DBL_LITERAL: return (nnc_i64)nnc_evald_dbl_literal(expr->exact);
        case EXPR_UNARY:       return nnc_evald_unary(expr->exact, st);
        case EXPR_BINARY:      return nnc_evald_binary(expr->exact, st);
        case EXPR_TERNARY:     return nnc_evald_ternary(expr->exact, st);
        default: {
            nnc_abort("nnc_evald: cannot evaluate this kind of expression.\n", nnc_expr_get_ctx(expr));
        }
    }
    return 0;
}