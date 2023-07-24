#ifndef _NNC_SYMTABLE_H
#define _NNC_SYMTABLE_H

#include "nnc_ast.h"
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

typedef struct _nnc_st {
    struct _nnc_st* root;
    struct _nnc_st** branches;
    _map_(nnc_str, nnc_fn_statement*) fns;
    _map_(nnc_str, nnc_let_statement*) vars;
    _map_(nnc_str, nnc_type*) types;
    _map_(nnc_str, nnc_fn_param*) params;
    _map_(nnc_str, nnc_namespace_statement*) namespaces;
    _map_(nnc_str, nnc_enumerator*) enumerators;
} nnc_st;

void nnc_st_init(nnc_st* out_table);
//todo: add finalizer

nnc_bool nnc_st_has_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key);
void nnc_st_put_entity(nnc_st* table, nnc_st_entity_kind kind, nnc_heap_ptr entity);
nnc_heap_ptr nnc_st_check_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key);
nnc_heap_ptr nnc_st_get_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key);

nnc_type* nnc_st_get_type(const nnc_heap_ptr entity, nnc_st_entity_kind kind);

//nnc_bool nnc_st_has_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key, nnc_st_traverse_type type);
//void nnc_st_put_entity(nnc_st* table, nnc_st_entity_kind kind, nnc_heap_ptr entity, nnc_st_traverse_type type);
//nnc_heap_ptr nnc_st_check_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key, nnc_st_traverse_type type);
//nnc_heap_ptr nnc_st_get_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key, nnc_st_traverse_type type);

#endif