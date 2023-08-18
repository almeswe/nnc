#ifndef _NNC_SYMTABLE_H
#define _NNC_SYMTABLE_H

#include "nnc_ast.h"
#include "nnc_literal.h"
#include "nnc_map.h"

typedef struct _nnc_type nnc_type;

typedef enum _nnc_st_traverse_type {
    ST_TRAVERSE_ALL,
    ST_TRAVERSE_UP,
    ST_TRAVERSE_DOWN,
    ST_TRAVERSE_ONE
} nnc_st_traverse_type;

typedef enum _nnc_symtable_entity_kind {
    ST_ENTITY_FN,
    ST_ENTITY_VAR,
    ST_ENTITY_TYPE,
    ST_ENTITY_PARAM,
    ST_ENTITY_NAMESPACE,
    ST_ENTITY_ENUMERATOR
} nnc_st_entity_kind;

typedef nnc_ident nnc_symbol;

typedef struct _nnc_st {
    struct _nnc_st* root;
    struct _nnc_st** branches;
    _map_(nnc_str, nnc_type*) types;
    _map_(nnc_str, nnc_symbol*) syms;
} nnc_st;

void nnc_st_init(nnc_st* out_table);

void nnc_st_put(nnc_st* table, nnc_symbol* sym);
void nnc_st_put_type(nnc_st* table, nnc_type* type);
nnc_bool nnc_st_has(nnc_st* table, const char* key);
nnc_bool nnc_st_has_type(nnc_st* table, const char* key);
nnc_symbol* nnc_st_get(nnc_st* table, const char* key);
nnc_type* nnc_st_get_type(nnc_st* table, const char* key); 
nnc_symbol* nnc_st_get_below(nnc_st* table, const char* key);

#endif