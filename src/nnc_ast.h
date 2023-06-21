#ifndef _NNC_AST_H
#define _NNC_AST_H

#include "nnc_statement.h"

typedef struct _nnc_ast {
    //todo: incomplete
    nnc_statement* root;
    const char* file;
} nnc_ast;

void nnc_dump_ast(const nnc_ast* ast);
nnc_ast* nnc_ast_new(const char* file);

#endif