#include "nnc_typecheck.h"

static nnc_type* nnc_chr_expr_infer_type() {
    return &u8_type;
}

static nnc_type* nnc_str_expr_infer_type() {
    return nnc_ptr_type_new(nnc_chr_expr_infer_type());
}

static nnc_type* nnc_dbl_expr_infer_type(nnc_dbl_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_F32: return &f32_type;
        case SUFFIX_F64: return &f64_type;
        default: nnc_abort_no_ctx("nnc_dbl_expr_infer_type: unknown suffix.\n");
    }
    return &unknown_type;
}

static nnc_type* nnc_int_expr_infer_type(nnc_int_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_I8:  return &i8_type;
        case SUFFIX_U8:  return &u8_type;
        case SUFFIX_I16: return &i16_type;
        case SUFFIX_U16: return &u16_type;
        case SUFFIX_I32: return &i32_type;
        case SUFFIX_U32: return &u32_type;
        case SUFFIX_I64: return &i64_type;
        case SUFFIX_U64: return &u64_type;
        default: nnc_abort_no_ctx("nnc_int_expr_infer_type: unknown suffix.\n");
    }
    return &unknown_type;
}

nnc_type* nnc_expr_infer_type(nnc_expression* expr) {
    printf("%u\n", expr->kind);
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: return nnc_chr_expr_infer_type();
        case EXPR_STR_LITERAL: return nnc_str_expr_infer_type();
        case EXPR_DBL_LITERAL: return nnc_dbl_expr_infer_type(expr->exact);
        case EXPR_INT_LITERAL: return nnc_int_expr_infer_type(expr->exact);
        default: nnc_abort_no_ctx("nnc_expr_infer_type: unknown kind.\n");
    }
    return &unknown_type;
}