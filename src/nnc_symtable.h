#ifndef _NNC_SYMTABLE_H
#define _NNC_SYMTABLE_H

#include "nnc_map.h"

#include "nnc_ast.h"
#include "nnc_literal.h"

typedef struct _nnc_type            nnc_type;
typedef struct _nnc_fn_statement    nnc_fn_statement;

typedef enum _nnc_st_sym_kind {
    ST_SYM_TYPE,
    ST_SYM_IDENT
} nnc_st_sym_kind;

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

typedef nnc_ident nnc_sym;

typedef struct _nnc_st {
    nnc_st_ctx ctx;
    struct _nnc_st* root;
    struct _nnc_st** branches;
    _map_(nnc_str, nnc_type*) types;
    _map_(nnc_str, nnc_sym*) syms;
    struct _nnc_st_ref {
        nnc_fn_statement* fn;
    } ref;
} nnc_st;

void nnc_st_init(nnc_st* out_st);
void nnc_st_fini(nnc_st* st);
void nnc_st_put(nnc_st* st, nnc_heap_ptr sym, nnc_st_sym_kind kind);

#define nnc_st_has_sym(st, key)    nnc_st_has(st, key, ST_SYM_IDENT)
#define nnc_st_has_type(st, key)   nnc_st_has(st, key, ST_SYM_TYPE)
nnc_bool nnc_st_has(const nnc_st* st, const char* key, nnc_st_sym_kind kind);

#define nnc_st_get_sym(st, sym)     (nnc_sym*)nnc_st_get(st, sym, ST_SYM_IDENT)
#define nnc_st_get_type(st, type)   (nnc_type*)nnc_st_get(st, type, ST_SYM_TYPE)
nnc_heap_ptr nnc_st_get(const nnc_st* st, const char* key, nnc_st_sym_kind kind);

nnc_bool nnc_st_has_ctx(const nnc_st* st, nnc_st** t_st, nnc_st_ctx ctx);

#endif