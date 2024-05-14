#ifndef __NNC_AST_H__
#define __NNC_AST_H__

#include "nnc_symtable.h"
#include "nnc_statement.h"

typedef struct _nnc_ast {
    nnc_statement** root;
    struct _nnc_st* st;
    const char* file;
} nnc_ast;

void nnc_dump_ast(
    FILE* fp,
    const nnc_ast* ast
);

nnc_ast* nnc_ast_new(
    const char* file
);

#endif