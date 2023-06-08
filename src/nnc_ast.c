#include "nnc_ast.h"

nnc_expression* nnc_expr_new(nnc_expression_kind kind, nnc_heap_ptr exact) {
    nnc_expression* ptr = new(nnc_expression);
    ptr->kind = kind;
    ptr->exact = exact;
    return ptr;
}

nnc_ast* nnc_ast_new(const char* file) {
    nnc_ast* ptr = new(nnc_ast);
    ptr->file = file;
    return ptr;
}