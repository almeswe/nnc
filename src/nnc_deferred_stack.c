#include "nnc_resolve.h"

void nnc_deferred_stack_init(nnc_deferred_stack* out_stack) {
    out_stack->entities = map_init_with(16);
    out_stack->meta = map_init_with(2);
}

void nnc_deferred_stack_fini(nnc_deferred_stack* stack) {
    if (stack->entities != NULL) {
        map_fini(stack->entities);
        stack->entities = NULL;
    }
    if (stack->meta != NULL) {
        map_fini(stack->meta);
        stack->meta = NULL;
    }
}

nnc_deferred_entity* nnc_deferred_entity_new(nnc_st* context, nnc_deferred_kind kind, nnc_heap_ptr exact) {
    nnc_deferred_entity* entity = anew(nnc_deferred_entity);
    entity->kind = kind;
    entity->exact = exact;
    entity->context = context;
    entity->status = STATUS_PUSHED;
    return entity;
}

void nnc_deferred_stack_put(nnc_st* context, nnc_deferred_kind kind, nnc_heap_ptr entity) {
    nnc_deferred_entity* deferred_entity = NULL;
    if (!map_has(glob_deferred_stack.entities, entity)) {
        deferred_entity = nnc_deferred_entity_new(context, kind, entity);
        map_put(glob_deferred_stack.entities, entity, deferred_entity);
    }
    deferred_entity = map_get(glob_deferred_stack.entities, entity);
    nnc_deferred_stack_update(deferred_entity);
}

void nnc_deferred_stack_meta_put(nnc_heap_ptr entity, nnc_deferred_meta* meta) {
    map_put(glob_deferred_stack.meta, entity, meta);
}

nnc_deferred_meta* nnc_deferred_meta_get(nnc_heap_ptr entity) {
    if (map_has(glob_deferred_stack.meta, entity)) {
        return map_get(glob_deferred_stack.meta, entity);
    }
    return NULL;
}

void nnc_deferred_stack_pop(nnc_heap_ptr entity) {
    map_pop(glob_deferred_stack.entities, entity);
    map_pop(glob_deferred_stack.meta, entity);
}

void nnc_deferred_stack_update(nnc_deferred_entity* entity) {
    const static nnc_deferred_status ceiling[] = {
        //todo: change this logic to smth with `lifetime`
        // also increase and descrease it when smth is resolved/not resolved
        [DEFERRED_TYPE]         = STATUS_INCOMPLETE2,
        [DEFERRED_EXPR]         = STATUS_INCOMPLETE2,
        [DEFERRED_SCOPE_EXPR]   = STATUS_INCOMPLETE2,
        [DEFERRED_UNARY_EXPR]   = STATUS_INCOMPLETE2,
        [DEFERRED_IDENT]        = STATUS_INCOMPLETE2,
        [DEFERRED_NAMESPACE]    = STATUS_INCOMPLETE2
    };
    if (entity->status >= ceiling[entity->kind]) {
        THROW(NNC_UNHANDLED, sformat("entity with kind %lu is unresolvable.", entity->kind));
    }
    entity->status += 1;
}

void nnc_deferred_stack_resolve() {
    nnc_deferred_entity* entity = NULL;
    nnc_map_bucket* buckets = glob_deferred_stack.entities->buckets;
    for (nnc_u64 i = 0; i < glob_deferred_stack.entities->cap; i++) {
        if (!buckets[i].has_key) {
            continue;
        }
        nnc_map_bucket* bucket = &buckets[i];
        for (; bucket != NULL; bucket = bucket->next) {
            assert(bucket->val != NULL);
            entity = map_get(glob_deferred_stack.entities, bucket->key);
            nnc_resolve_entity(entity);
        }
    }
}