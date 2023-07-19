#include "nnc_deferred_stack.h"

typedef nnc_bool (nnc_resolve_fn)(nnc_deferred_entity*);

static nnc_bool nnc_resolve_var(nnc_deferred_entity* entity) {
    const nnc_st* context = entity->context;
    nnc_ident* ident = entity->exact;
    nnc_heap_ptr st_entity = nnc_st_get_entity(context, ST_ENTITY_VAR, ident->name);
    if (st_entity != NULL && ((nnc_let_statement*)st_entity)->is_topmost) {
        ident->type = ((nnc_let_statement*)st_entity)->type;
        return true;
    }
    st_entity = nnc_st_get_entity(context, ST_ENTITY_FN_PARAM, ident->name);
    if (st_entity != NULL) {
        ident->type = ((nnc_fn_param*)st_entity)->type;
        return true;
    }
    //todo: add enum member check
    return st_entity != NULL;
}

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity) {
    static nnc_resolve_fn* resolve[] = {
        [ST_ENTITY_VAR] = nnc_resolve_var
    };
    return resolve[entity->kind](entity);
}