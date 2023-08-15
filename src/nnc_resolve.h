#ifndef _NNC_SEMANTIC_H
#define _NNC_SEMANTIC_H

#include "nnc_symtable.h"

typedef enum _nnc_deferred_status {
    STATUS_PUSHED           = 0,
    STATUS_INCOMPLETE1      = 1,
    STATUS_INCOMPLETE2      = 2,
    STATUS_INCOMPLETE3      = 3,
    STATUS_UNRESOLVABLE     = 4
} nnc_deferred_status;

typedef enum _nnc_deferred_kind {
    DEFERRED_FN,
    DEFERRED_EXPR,
    DEFERRED_UNARY_EXPR,
    DEFERRED_BINARY_EXPR,
    DEFERRED_IDENT,
    DEFERRED_PARAM,
    DEFERRED_NAMESPACE,
    DEFERRED_ENUMERATOR,
    DEFERRED_DOT_EXPR,
    DEFERRED_SCOPE_EXPR
} nnc_deferred_kind;

typedef struct _nnc_deferred_meta {
    nnc_ident* in_namespace;
} nnc_deferred_meta;

typedef struct _nnc_deferred_entity {
    nnc_st* context;
    nnc_deferred_kind kind;
    nnc_deferred_status status;
    nnc_heap_ptr exact;
} nnc_deferred_entity;

typedef struct _nnc_deferred_stack {
    _map_(nnc_heap_ptr, nnc_deferred_entity*) entities;
    _map_(nnc_heap_ptr, nnc_deferred_meta) meta;
} nnc_deferred_stack;

extern nnc_deferred_stack glob_deferred_stack;

void nnc_deferred_stack_init(nnc_deferred_stack* out_stack);
void nnc_deferred_stack_fini(nnc_deferred_stack* stack);

nnc_deferred_entity* nnc_deferred_entity_new(nnc_st* context, nnc_deferred_kind kind, nnc_heap_ptr exact);

void nnc_deferred_stack_put(nnc_st* context, nnc_deferred_kind kind, nnc_heap_ptr entity);
void nnc_deferred_stack_pop(nnc_heap_ptr entity);

void nnc_deferred_stack_update(nnc_deferred_entity* entity);

void nnc_deferred_stack_resolve();

void nnc_deferred_stack_meta_put(nnc_heap_ptr entity, nnc_deferred_meta* meta);
nnc_deferred_meta* nnc_deferred_meta_get(nnc_heap_ptr entity);

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity);
nnc_bool nnc_resolve_expr(nnc_expression* expr, nnc_st* table);

#endif