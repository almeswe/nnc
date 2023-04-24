#ifndef _NNC_ERROR_H
#define _NNC_ERROR_H

#include "nnc_arena.h"

typedef struct _nnc_warning {
    char placeholder;
} nnc_warning;

typedef struct _nnc_error {
    char placeholder;
} nnc_error;

typedef struct _nnc_report {
    nnc_error* errors;
    nnc_warning* warnings;
} nnc_report;

void nnc_errno(char* fmt, ...);

// [ x ] Lexing...
// [ x ] Parsing...
// [ x ] Ast resolving... [pthreads: 5]
// [ x ] Generating C code...
// [ x ] Compiling C code...
// [ x ] Done. [memory: 6134 bytes]
// [ x ] Exit code: 0

/*
static void nnc_visit_fn_call(nnc_expr* fn_call);
static void nnc_visit_while(nnc_stmt* loop);
static void nnc_visit_for(nnc_stmt* loop);

static void nnc_visit_typedef(nnc_stmt* tdef) {
}

static void nnc_visit_fn_param(nnc_fn_param* param) {
    nnc_visit_type(param->type);
    nnc_visit_name(param->idnt);
}

static void nnc_visit_fn_decl(nnc_stmt* fn) {

}

static void nnc_visit_namespace(nnc_stmt* namespace) {
    
}

nnc_report* nnc_visit_ast(nnc_ast* ast) {
    nnc_report* report = nnc_report_init();
    for (nnc_u64 i = 0; buf_len(ast->namespaces); i++) {
    }
    return report;
}

*/


#endif