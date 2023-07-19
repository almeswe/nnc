#include "nnc_deferred_stack.h"

void nnc_deferred_stack_init(nnc_deferred_stack* out_stack) {
    out_stack->entities = map_init_with(16);
}

void nnc_deferred_stack_fini(nnc_deferred_stack* stack) {
    if (stack->entities != NULL) {
        map_fini(stack->entities);
        stack->entities = NULL;
    }
}

nnc_deferred_entity* nnc_deferred_entity_new(nnc_st* context, nnc_st_entity_kind kind, nnc_heap_ptr exact) {
    nnc_deferred_entity* entity = new(nnc_deferred_entity);
    entity->kind = kind;
    entity->exact = exact;
    entity->context = context;
    entity->status = STATUS_PUSHED;
    return entity;
}

void nnc_deferred_stack_put(nnc_deferred_stack* stack, nnc_deferred_entity* entity) {
    if (!map_has(stack->entities, entity->exact)) {
        map_put(stack->entities, entity->exact, entity);
    }
    if (nnc_resolve_entity(entity)) {
        nnc_deferred_stack_pop(stack, entity);
    }
    else {
        nnc_deferred_stack_update(stack, entity);
    }
}

void nnc_deferred_stack_pop(nnc_deferred_stack* stack, nnc_deferred_entity* entity) {
    map_pop(stack->entities, entity->exact);
}

void nnc_deferred_stack_update(nnc_deferred_stack* stack, nnc_deferred_entity* entity) {
    const static nnc_deferred_status ceiling[] = {
        [ST_ENTITY_VAR] = STATUS_INCOMPLETE2,
    };
    if (entity->status >= ceiling[entity->kind]) {
        THROW(NNC_UNHANDLED, sformat("entity with kind %lu is unresolvable.", entity->kind));
    }
    entity->status += 1;
}

void nnc_deferred_stack_resolve(nnc_deferred_stack* stack) {
    nnc_map_bucket* buckets = stack->entities->buckets;
    for (nnc_u64 i = 0; i < stack->entities->cap; i++) {
        if (!buckets[i].has_key) {
            continue;
        }
        nnc_map_bucket* bucket = &buckets[i];
        for (; bucket != NULL; bucket = bucket->next) {
            assert(bucket->val != NULL);
            nnc_deferred_stack_put(stack, bucket->val);
        }
    }
}