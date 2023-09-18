#include "nnc_ast.h"

nnc_ident* nnc_ident_new(const nnc_byte* from, const nnc_ctx* ctx) {
    nnc_ident* ptr = anew(nnc_ident);
    ptr->type = &unknown_type;
    ptr->size = strlen(from);
    ptr->name = cnew(nnc_byte, ptr->size + 1);
    ptr->ictx = IDENT_DEFAULT;
    if (ctx != NULL) {
        ptr->ctx = *ctx;
    }
    strcpy(ptr->name, from);
    return ptr;
}

nnc_ctx* nnc_expr_get_ctx(const nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: return &((nnc_chr_literal*)expr->exact)->ctx;        break;
        case EXPR_STR_LITERAL: return &((nnc_str_literal*)expr->exact)->ctx;        break;
        case EXPR_DBL_LITERAL: return &((nnc_dbl_literal*)expr->exact)->ctx;        break;
        case EXPR_INT_LITERAL: return &((nnc_int_literal*)expr->exact)->ctx;        break;
        case EXPR_IDENT:       return &((nnc_ident*)expr->exact)->ctx;              break;
        case EXPR_UNARY:       return &((nnc_unary_expression*)expr->exact)->ctx;   break;
        case EXPR_BINARY:      return &((nnc_binary_expression*)expr->exact)->ctx;  break;
        case EXPR_TERNARY:     return &((nnc_ternary_expression*)expr->exact)->ctx; break;
        default: nnc_abort_no_ctx("nnc_expr_get_ctx: unknown kind.\n");
    }
    return NULL;
}

nnc_unary_expression* nnc_unary_expr_new(nnc_unary_expression_kind kind) {
    nnc_unary_expression* ptr = anew(nnc_unary_expression);
    ptr->type = &unknown_type;
    ptr->kind = kind;
    return ptr;
}

nnc_binary_expression* nnc_binary_expr_new(nnc_binary_expression_kind kind) {
    nnc_binary_expression* ptr = anew(nnc_binary_expression);
    ptr->type = &unknown_type;
    ptr->kind = kind;
    return ptr;
}

nnc_ternary_expression* nnc_ternary_expr_new() {
    nnc_ternary_expression* ptr = anew(nnc_ternary_expression);
    ptr->type = &unknown_type;
    return ptr;
}

nnc_expression* nnc_expr_new(nnc_expression_kind kind, nnc_heap_ptr exact) {
    nnc_expression* ptr = anew(nnc_expression);
    ptr->kind = kind;
    ptr->exact = exact;
    return ptr;
}

nnc_statement* nnc_stmt_new(nnc_statement_kind kind, nnc_heap_ptr exact) {
    nnc_statement* ptr = anew(nnc_statement);
    ptr->kind = kind;
    ptr->exact = exact;
    return ptr;
}

nnc_ast* nnc_ast_new(const char* file) {
    nnc_ast* ptr = anew(nnc_ast);
    ptr->file = file;
    return ptr;
}