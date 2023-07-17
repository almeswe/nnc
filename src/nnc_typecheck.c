#include "nnc_typecheck.h"

static nnc_type* nnc_chr_expr_infer_type(nnc_chr_literal* literal) {
    return literal->type = &u8_type;
}

static nnc_type* nnc_str_expr_infer_type(nnc_str_literal* literal) {
    return literal->type = nnc_ptr_type_new(&u8_type);
}

static nnc_type* nnc_dbl_expr_infer_type(nnc_dbl_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_F32: return literal->type = &f32_type;
        case SUFFIX_F64: return literal->type = &f64_type;
        default: nnc_abort_no_ctx("nnc_dbl_expr_infer_type: unknown suffix.\n");
    }
    return &unknown_type;
}

static nnc_type* nnc_int_expr_infer_type(nnc_int_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_I8:  return literal->type = &i8_type;
        case SUFFIX_U8:  return literal->type = &u8_type;
        case SUFFIX_I16: return literal->type = &i16_type;
        case SUFFIX_U16: return literal->type = &u16_type;
        case SUFFIX_I32: return literal->type = &i32_type;
        case SUFFIX_U32: return literal->type = &u32_type;
        case SUFFIX_I64: return literal->type = &i64_type;
        case SUFFIX_U64: return literal->type = &u64_type;
        default: nnc_abort_no_ctx("nnc_int_expr_infer_type: unknown suffix.\n");
    }
    return &unknown_type;
}

nnc_type* nnc_expr_infer_type(nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: return nnc_chr_expr_infer_type(expr->exact);
        case EXPR_STR_LITERAL: return nnc_str_expr_infer_type(expr->exact);
        case EXPR_DBL_LITERAL: return nnc_dbl_expr_infer_type(expr->exact);
        case EXPR_INT_LITERAL: return nnc_int_expr_infer_type(expr->exact);
        default: nnc_abort_no_ctx("nnc_expr_infer_type: unknown kind.\n");
    }
    return &unknown_type;
}

nnc_type* nnc_expr_get_type(nnc_expression* expr) {
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