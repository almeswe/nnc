#include "nnc_ast_eval.h"

nnc_static nnc_i64 nnc_evald_ident(const nnc_ident* ident, const nnc_st* table) {
    assert(ident->ctx == IDENT_ENUMERATOR);
    nnc_symbol* sym = nnc_st_get(table, ident->name);
    assert(sym != NULL && sym->ctx == IDENT_ENUMERATOR);
    return sym->refs.enumerator->init_const.d;
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

nnc_static nnc_i64 nnc_evald_binary(const nnc_binary_expression* binary, const nnc_st* table) {
    switch (binary->kind) {
        case BINARY_OR:     return nnc_evald(binary->lexpr, table) || nnc_evald(binary->rexpr, table);
        case BINARY_LT:     return nnc_evald(binary->lexpr, table) < nnc_evald(binary->rexpr, table);
        case BINARY_GT:     return nnc_evald(binary->lexpr, table) > nnc_evald(binary->rexpr, table);
        case BINARY_EQ:     return nnc_evald(binary->lexpr, table) == nnc_evald(binary->rexpr, table);
        case BINARY_ADD:    return nnc_evald(binary->lexpr, table) + nnc_evald(binary->rexpr, table);
        case BINARY_SUB:    return nnc_evald(binary->lexpr, table) - nnc_evald(binary->rexpr, table);
        case BINARY_MUL:    return nnc_evald(binary->lexpr, table) * nnc_evald(binary->rexpr, table);
        case BINARY_DIV:    return nnc_evald(binary->lexpr, table) / nnc_evald(binary->rexpr, table);
        case BINARY_MOD:    return nnc_evald(binary->lexpr, table) % nnc_evald(binary->rexpr, table);
        case BINARY_SHR:    return nnc_evald(binary->lexpr, table) >> nnc_evald(binary->rexpr, table);
        case BINARY_SHL:    return nnc_evald(binary->lexpr, table) << nnc_evald(binary->rexpr, table);
        case BINARY_NEQ:    return nnc_evald(binary->lexpr, table) != nnc_evald(binary->rexpr, table);
        case BINARY_AND:    return nnc_evald(binary->lexpr, table) && nnc_evald(binary->rexpr, table);
        case BINARY_LTE:    return nnc_evald(binary->lexpr, table) <= nnc_evald(binary->rexpr, table);
        case BINARY_GTE:    return nnc_evald(binary->lexpr, table) >= nnc_evald(binary->rexpr, table);
        case BINARY_COMMA:  return nnc_evald(binary->rexpr, table);
        case BINARY_BW_OR:  return nnc_evald(binary->lexpr, table) | nnc_evald(binary->rexpr, table);
        case BINARY_BW_AND: return nnc_evald(binary->lexpr, table) & nnc_evald(binary->rexpr, table);
        case BINARY_BW_XOR: return nnc_evald(binary->lexpr, table) ^ nnc_evald(binary->rexpr, table);
        default: nnc_abort_no_ctx("nnc_evald_binary: cannot evaluate this kind of expression.");
    }
    return 0;
}

nnc_static nnc_i64 nnc_evald_ternary(const nnc_ternary_expression* ternary, const nnc_st* table) {
    if (nnc_evald(ternary->cexpr, table)) {
        return nnc_evald(ternary->lexpr, table);
    }
    return nnc_evald(ternary->rexpr, table);
}

nnc_i64 nnc_evald(const nnc_expression* expr, const nnc_st* table) {
    switch (expr->kind) {
        case EXPR_IDENT:       return nnc_evald_ident(expr->exact, table);  
        case EXPR_INT_LITERAL: return nnc_evald_int_literal(expr->exact);
        case EXPR_CHR_LITERAL: return nnc_evald_chr_literal(expr->exact);
        case EXPR_DBL_LITERAL: return (nnc_i64)nnc_evald_dbl_literal(expr->exact);
        case EXPR_BINARY:      return nnc_evald_binary(expr->exact, table);
        case EXPR_TERNARY:     return nnc_evald_ternary(expr->exact, table);
        default: nnc_abort_no_ctx("nnc_evald: cannot evaluate this kind of expression.");
    }
    return 0;
}