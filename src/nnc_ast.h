#ifndef _NNC_AST_H
#define _NNC_AST_H

#include "nnc_symtable.h"
#include "nnc_statement.h"

typedef struct _nnc_ast {
    nnc_statement** root;
    struct _nnc_st* st;
    const char* file;
} nnc_ast;

void nnc_dump_ast(const nnc_ast* ast);
nnc_ast* nnc_ast_new(const char* file);

#endif