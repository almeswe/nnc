#include "nnc_symtable.h"

void nnc_st_init(nnc_st* out_table) {
    out_table->fns          = map_init_with(1);
    out_table->vars         = map_init_with(1);
    out_table->types        = map_init_with(1);
    out_table->params       = map_init_with(1);
    out_table->enum_members = map_init_with(1);
}

static nnc_bool nnc_st_has_var(nnc_st* table, nnc_str key) {
    return map_has_s(table->vars, key);
}

static nnc_bool nnc_st_has_fn(nnc_st* table, nnc_str key) {
    return map_has_s(table->fns, key);
}

static nnc_bool nnc_st_has_fn_param(nnc_st* table, nnc_str key) {
    return map_has_s(table->params, key);
}

nnc_bool nnc_st_has_entity(nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    switch (kind) {
        case ST_ENTITY_VAR:      return nnc_st_has_var(table, key);
        case ST_ENTITY_FN:       return nnc_st_has_fn(table, key);
        case ST_ENTITY_FN_PARAM: return nnc_st_has_fn_param(table, key);
        default: nnc_abort_no_ctx("nnc_st_has_entity: kind is unknown.\n");
    }
    return false;
}

static void nnc_st_put_var(nnc_st* table, nnc_heap_ptr entity) {
    nnc_let_statement* let_stmt = (nnc_let_statement*)entity;
    nnc_str name = let_stmt->var->name;
    map_put_s(table->vars, name, let_stmt);
}

static void nnc_st_try_put_var(nnc_st* table, nnc_heap_ptr entity) {
    nnc_st* origin = table;
    nnc_str name = ((nnc_let_statement*)entity)->var->name;
    for (; table != NULL; table = table->root) {
        if (nnc_st_has_var(table, name)) {
            THROW(NNC_UNHANDLED, sformat("variable \'%s\' is alredy declared.", name));
        }
        if (nnc_st_has_fn_param(table, name)) {
            THROW(NNC_UNHANDLED, sformat("cannot declare variable \'%s\', parameter with this name is already declared.", name));
        }
    }
    nnc_st_put_var(origin, entity);
}

static void nnc_st_put_fn_param(nnc_st* table, nnc_heap_ptr entity) {
    nnc_fn_param* fn_param = (nnc_fn_param*)entity;
    nnc_str name = fn_param->var->name;
    map_put_s(table->params, name, fn_param);
}

static void nnc_st_try_put_fn_param(nnc_st* table, nnc_heap_ptr entity) {
    nnc_st* origin = table;
    nnc_str name = ((nnc_fn_param*)entity)->var->name;
    for (; table != NULL; table = table->root) {
        if (nnc_st_has_fn_param(table, name)) {
            THROW(NNC_UNHANDLED, sformat("function parameter \'%s\' is alredy declared.", name));
        }
        if (nnc_st_has_var(table, name)) {
            THROW(NNC_UNHANDLED, sformat("cannot declare parameter \'%s\', variable with this name is already declared.", name));
        }
    }
    nnc_st_put_fn_param(origin, entity);
}

static void nnc_st_put_fn(nnc_st* table, nnc_heap_ptr entity) {
    nnc_fn_statement* fn_stmt = (nnc_fn_statement*)entity;
    nnc_str name = fn_stmt->var->name;
    map_put_s(table->fns, name, fn_stmt);
}   

static void nnc_st_try_put_fn(nnc_st* table, nnc_heap_ptr entity) {
    nnc_st* origin = table;
    nnc_str name = ((nnc_fn_statement*)entity)->var->name;
    for (; table != NULL; table = table->root) {
        if (nnc_st_has_fn(table, name)) {
            THROW(NNC_UNHANDLED, sformat("function \'%s\' is alredy declared.", name));
        }
    }
    nnc_st_put_fn(origin, entity);
}

void nnc_st_put_entity(nnc_st* table, nnc_st_entity_kind kind, nnc_heap_ptr entity) {
    if (entity == NULL) {
        THROW(NNC_UNHANDLED, "nnc_st_put_entity: entity is NULL.");
    }
    switch (kind) {
        case ST_ENTITY_VAR:      nnc_st_try_put_var(table, entity);      break;
        case ST_ENTITY_FN:       nnc_st_try_put_fn(table, entity);       break;
        case ST_ENTITY_FN_PARAM: nnc_st_try_put_fn_param(table, entity); break;
        default: nnc_abort_no_ctx("nnc_st_put_entity: kind is unknown.\n");
    }
}

static nnc_let_statement* nnc_st_get_var(nnc_st* table, nnc_str key) {
    if (nnc_st_has_var(table, key)) {
        return (nnc_let_statement*)map_get_s(table->vars, key);
    }
    return NULL;
}

static nnc_fn_statement* nnc_st_get_fn(nnc_st* table, nnc_str key) {
    if (nnc_st_has_fn(table, key)) {
        return (nnc_fn_statement*)map_get_s(table->fns, key);
    }
    return NULL;
}

static nnc_fn_param* nnc_st_get_fn_param(nnc_st* table, nnc_str key) {
    if (nnc_st_has_fn_param(table, key)) {
        return (nnc_fn_param*)map_get_s(table->params, key);
    }
    return NULL;
}

nnc_heap_ptr nnc_st_get_entity(nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    switch (kind) {
        case ST_ENTITY_VAR:      return nnc_st_get_var(table, key);
        case ST_ENTITY_FN:       return nnc_st_get_fn(table, key);
        case ST_ENTITY_FN_PARAM: return nnc_st_get_fn_param(table, key);
        default: nnc_abort_no_ctx("nnc_st_get_entity: kind is unknown.\n");
    }
    return NULL;
}