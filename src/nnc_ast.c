#include "nnc_ast.h"

nnc_dbl_literal* nnc_dbl_new(const char* repr) {
    nnc_dbl_literal* ptr = new(nnc_dbl_literal);
    ptr->endian = ENDIAN_F64;
    ptr->exact = atof(repr);
    return ptr;
}

nnc_int_literal* nnc_int_new(const char* repr) {
    nnc_int_literal* ptr = new(nnc_int_literal);
    ptr->base = 10;
    ptr->has_sign = false;
    ptr->endian = ENDIAN_U64;
    switch (repr[0]) {
        case 'x': ptr->base = 16; break;
        case 'o': ptr->base = 8;  break;
        case 'b': ptr->base = 2;  break;
    }
    if (ptr->base != 10) {
        repr = repr + 1;
    }
    ptr->exact.d = strtoll(repr, NULL, ptr->base);
    if (errno != 0) {
        nnc_abort_no_ctx(sformat("nnc_int_new [errno: %d]: cannot"
            " instanciate with \'%s\'\n", errno, repr));
    }
    return ptr;
}

nnc_chr_literal* nnc_chr_new(const char* repr) {
    THROW(NNC_UNINPLEMENTED, "nnc_chr_new\n");
    return NULL;
}

nnc_str_literal* nnc_str_new(const char* repr) {
    THROW(NNC_UNINPLEMENTED, "nnc_str_new\n");
    return NULL;
}

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