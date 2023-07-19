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

typedef struct _nnc_deferred_entity {
    nnc_st* context;
    nnc_st_entity_kind kind;
    nnc_deferred_status status;
    nnc_heap_ptr exact;
} nnc_deferred_entity;

typedef struct _nnc_deferred_stack {
    _map_(nnc_heap_ptr, nnc_deferred_entity*) entities;
} nnc_deferred_stack;

extern nnc_deferred_stack glob_deferred_stack;

void nnc_deferred_stack_init(nnc_deferred_stack* out_stack);
void nnc_deferred_stack_fini(nnc_deferred_stack* stack);

nnc_deferred_entity* nnc_deferred_entity_new(nnc_st* context, nnc_st_entity_kind kind, nnc_heap_ptr exact);

void nnc_deferred_stack_put(nnc_deferred_stack* stack, nnc_deferred_entity* entity);
void nnc_deferred_stack_pop(nnc_deferred_stack* stack, nnc_deferred_entity* entity);

void nnc_deferred_stack_update(nnc_deferred_stack* stack, nnc_deferred_entity* entity);

void nnc_deferred_stack_resolve(nnc_deferred_stack* stack);

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity);

#endif