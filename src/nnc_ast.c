#include "nnc_ast.h"

nnc_ident* nnc_ident_new(const nnc_byte* from) {
    nnc_ident* ptr = new(nnc_ident);
    ptr->type = &unknown_type;
    ptr->size = strlen(from);
    ptr->name = cnew(nnc_byte, ptr->size + 1);
    ptr->ctx = IDENT_DEFAULT;
    strcpy(ptr->name, from);
    return ptr;
}

void nnc_ident_set_ctx(nnc_ident* ident, nnc_ident_ctx ctx) {
    ident->ctx = ctx;
}

nnc_unary_expression* nnc_unary_expr_new(nnc_unary_expression_kind kind) {
    nnc_unary_expression* ptr = new(nnc_unary_expression);
    ptr->type = &unknown_type;
    ptr->kind = kind;
    return ptr;
}

nnc_binary_expression* nnc_binary_expr_new(nnc_binary_expression_kind kind) {
    nnc_binary_expression* ptr = new(nnc_binary_expression);
    ptr->type = &unknown_type;
    ptr->kind = kind;
    return ptr;
}

nnc_ternary_expression* nnc_ternary_expr_new() {
    nnc_ternary_expression* ptr = new(nnc_ternary_expression);
    ptr->type = &unknown_type;
    return ptr;
}

nnc_expression* nnc_expr_new(nnc_expression_kind kind, nnc_heap_ptr exact) {
    nnc_expression* ptr = new(nnc_expression);
    ptr->kind = kind;
    ptr->exact = exact;
    ptr->ctx = EXPR_CTX_NONE;
    return ptr;
}

nnc_expression* nnc_expr_ctx_new(nnc_expression_kind kind, nnc_expression_ctx ctx, nnc_heap_ptr exact) {
    nnc_expression* expr = nnc_expr_new(kind, exact);
    expr->ctx = ctx;
    return expr;
}

nnc_statement* nnc_stmt_new(nnc_statement_kind kind, nnc_heap_ptr exact) {
    nnc_statement* ptr = new(nnc_statement);
    ptr->kind = kind;
    ptr->exact = exact;
    return ptr;
}

nnc_ast* nnc_ast_new(const char* file) {
    nnc_ast* ptr = new(nnc_ast);
    ptr->file = file;
    return ptr;
}