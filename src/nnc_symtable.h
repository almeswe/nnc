#ifndef _NNC_SYMTABLE_H
#define _NNC_SYMTABLE_H

#include "nnc_ast.h"
#include "nnc_literal.h"
#include "nnc_map.h"

typedef nnc_ident nnc_symbol;
typedef struct _nnc_type nnc_type;

typedef struct _nnc_fn_statement nnc_fn_statement;
typedef struct _nnc_namespace_statement nnc_namespace_statement;

typedef enum _nnc_st_ctx {
    ST_CTX_DEFAULT   = 0x00000000,
    ST_CTX_GLOBAL    = 0x00000001,
    ST_CTX_FN        = 0x00000010,
    ST_CTX_LOOP      = 0x00000100,
    ST_CTX_NAMESPACE = 0x00001000,
    ST_CTX_IF        = 0x00010000,
    ST_CTX_ELIF      = 0x00100000,
    ST_CTX_ELSE      = 0x01000000
} nnc_st_ctx;

typedef struct _nnc_st {
    nnc_st_ctx ctx;
    struct _nnc_st* root;
    struct _nnc_st** branches;
    _map_(nnc_str, nnc_type*) types;
    _map_(nnc_str, nnc_symbol*) syms;
    struct _nnc_st_ref {
        nnc_fn_statement* fn;
    } ref;
} nnc_st;

nnc_st* nnc_st_new();
void nnc_st_init(nnc_st* out_table);
void nnc_st_put(nnc_st* st, nnc_symbol* sym);
void nnc_st_put_type(nnc_st* st, nnc_type* type);
nnc_bool nnc_st_has(const nnc_st* st, const char* key);
nnc_bool nnc_st_has_type(const nnc_st* st, const char* key);
nnc_symbol* nnc_st_get(const nnc_st* st, const char* key);
nnc_type* nnc_st_get_type(const nnc_st* st, const char* key); 
nnc_symbol* nnc_st_get_below(const nnc_st* st, const char* key);

#endif