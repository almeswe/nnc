#include "nnc_symtable.h"

/*
    rules for name collision check

    fn:             another fn, upper variable.
    namespace:      another namespace, upper fn or variable
    variable:       another variable, namespace, parameter, fn
    parameter:      another parameter, upper variable, namespace, fn
    enum member:    another enum member in this enum
    struct member:  another struct member in this struct or union
*/

void nnc_st_init(nnc_st* out_table) {
    out_table->fns         = map_init_with(1);
    out_table->vars        = map_init_with(1);
    out_table->types       = map_init_with(1);
    out_table->params      = map_init_with(1);
    out_table->namespaces  = map_init_with(1);
    out_table->enumerators = map_init_with(1);
}

static nnc_map* nnc_st_get_map(const nnc_st* table, nnc_st_entity_kind kind) {
    switch (kind) {
        case ST_ENTITY_FN:         return table->fns;
        case ST_ENTITY_VAR:        return table->vars;
        case ST_ENTITY_TYPE:       return table->types;
        case ST_ENTITY_PARAM:      return table->params;
        case ST_ENTITY_NAMESPACE:  return table->namespaces;
        case ST_ENTITY_ENUMERATOR: return table->enumerators;
        default: nnc_abort_no_ctx("nnc_st_get_map: unknown kind.");
    }
    return NULL;
}

static nnc_bool nnc_has_across_tables(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    nnc_bool has_in_map = false;
    for (; table != NULL && !has_in_map; table = table->root) {
        nnc_map* map = nnc_st_get_map(table, kind);
        has_in_map |= map_has_s(map, key);
    }
    return has_in_map;
}

static nnc_heap_ptr nnc_get_across_tables(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    nnc_heap_ptr entity = NULL;
    for (; table != NULL; table = table->root) {
        nnc_map* map = nnc_st_get_map(table, kind);
        if (map_has_s(map, key)) {
            entity = map_get_s(map, key);
        }
    }
    return entity;
}

static nnc_bool nnc_st_has_fn(const nnc_st* table, nnc_str key) {
    return nnc_has_across_tables(table, ST_ENTITY_FN, key);
}

static nnc_bool nnc_st_has_var(const nnc_st* table, nnc_str key) {
    return nnc_has_across_tables(table, ST_ENTITY_VAR, key);
}

static nnc_bool nnc_st_has_type(const nnc_st* table, nnc_str key) {
    return nnc_has_across_tables(table, ST_ENTITY_TYPE, key);
}

static nnc_bool nnc_st_has_namespace(const nnc_st* table, nnc_str key) {
    return nnc_has_across_tables(table, ST_ENTITY_NAMESPACE, key);
}

static nnc_bool nnc_st_has_param(const nnc_st* table, nnc_str key) {
    return nnc_has_across_tables(table, ST_ENTITY_PARAM, key);
}

static nnc_bool nnc_st_has_enumerator(const nnc_st* table, nnc_str key) {
    return nnc_has_across_tables(table, ST_ENTITY_ENUMERATOR, key);
}

nnc_bool nnc_st_has_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    switch (kind) {
        case ST_ENTITY_FN:         return nnc_st_has_fn(table, key);
        case ST_ENTITY_VAR:        return nnc_st_has_var(table, key);
        case ST_ENTITY_TYPE:       return nnc_st_has_type(table, key);
        case ST_ENTITY_PARAM:      return nnc_st_has_param(table, key);
        case ST_ENTITY_NAMESPACE:  return nnc_st_has_namespace(table, key);
        case ST_ENTITY_ENUMERATOR: return nnc_st_has_enumerator(table, key);
        default: nnc_abort_no_ctx("nnc_st_has_entity: kind is unknown.\n");
    }
    return false;
}

nnc_heap_ptr nnc_st_check_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    const static char* entity_str[] = {
        [ST_ENTITY_FN]          = "function",
        [ST_ENTITY_VAR]         = "variable",
        [ST_ENTITY_TYPE]        = "type",
        [ST_ENTITY_PARAM]       = "parameter",
        [ST_ENTITY_NAMESPACE]   = "namespace",
        [ST_ENTITY_ENUMERATOR]  = "enumerator"
    };
    if (!nnc_st_has_entity(table, kind, key)) {
        THROW(NNC_TABLE_MISS, sformat("undeclared %s \'%s\' met.", entity_str[kind], key));
    }
    return nnc_st_get_entity(table, kind, key);
}

nnc_heap_ptr nnc_st_get_entity(const nnc_st* table, nnc_st_entity_kind kind, nnc_str key) {
    if (nnc_st_has_entity(table, kind, key)) {
        return nnc_get_across_tables(table, kind, key);
    }
    return NULL;
}

static void nnc_st_put_fn(nnc_st* table, nnc_heap_ptr entity) {
    nnc_fn_statement* fn_stmt = (nnc_fn_statement*)entity;
    nnc_str name = fn_stmt->var->name;
    map_put_s(table->fns, name, fn_stmt);
}   

static void nnc_st_try_put_fn(nnc_st* table, nnc_heap_ptr entity) {
    nnc_str name = ((nnc_fn_statement*)entity)->var->name;
    if (nnc_st_has_fn(table, name)) {
        THROW(NNC_UNHANDLED, sformat("function \'%s\' is already declared.", name));
    }
    nnc_st_put_fn(table, entity);
}

static void nnc_st_put_var(nnc_st* table, nnc_heap_ptr entity) {
    nnc_let_statement* let_stmt = (nnc_let_statement*)entity;
    nnc_str name = let_stmt->var->name;
    map_put_s(table->vars, name, let_stmt);
}

static void nnc_st_try_put_var(nnc_st* table, nnc_heap_ptr entity) {
    nnc_str name = ((nnc_let_statement*)entity)->var->name;
    if (nnc_st_has_var(table, name)) {
        THROW(NNC_UNHANDLED, sformat("variable \'%s\' is already declared.", name));
    }
    if (nnc_st_has_param(table, name)) {
        THROW(NNC_UNHANDLED, sformat("cannot declare variable \'%s\', parameter with this name is already declared.", name));
    }
    if (nnc_st_has_entity(table, ST_ENTITY_ENUMERATOR, name)) {
        THROW(NNC_UNHANDLED, sformat("cannot declare variable with name \'%s\', "
            "enum member with this name is already declared.", name));
    }
    nnc_st_put_var(table, entity);
}

static void nnc_st_put_type(nnc_st* table, nnc_heap_ptr entity) {
    nnc_type* type = (nnc_type*)entity;
    nnc_str name = type->repr;
    map_put_s(table->types, name, type);
}

static void nnc_st_try_put_type(nnc_st* table, nnc_heap_ptr entity) {
    nnc_str name = ((nnc_type*)entity)->repr;
    if (nnc_st_has_type(table, name)) {
        THROW(NNC_UNHANDLED, sformat("type \'%s\' is alredy declared.", name));
    }
    nnc_st_put_type(table, entity);
}

static void nnc_st_put_fn_param(nnc_st* table, nnc_heap_ptr entity) {
    nnc_fn_param* fn_param = (nnc_fn_param*)entity;
    nnc_str name = fn_param->var->name;
    map_put_s(table->params, name, fn_param);
}

static void nnc_st_try_put_param(nnc_st* table, nnc_heap_ptr entity) {
    nnc_str name = ((nnc_fn_param*)entity)->var->name;
    if (nnc_st_has_param(table, name)) {
        THROW(NNC_UNHANDLED, sformat("function parameter \'%s\' is already declared.", name));
    }
    if (nnc_st_has_var(table, name)) {
        THROW(NNC_UNHANDLED, sformat("cannot declare parameter \'%s\', variable with this name is already declared.", name));
    }
    nnc_st_put_fn_param(table, entity);
}

static void nnc_st_put_namespace(nnc_st* table, nnc_heap_ptr entity) {
    nnc_namespace_statement* namespace_stmt = (nnc_namespace_statement*)entity;
    nnc_str name = namespace_stmt->var->name;
    map_put_s(table->namespaces, name, namespace_stmt);
}

static void nnc_st_try_put_namespace(nnc_st* table, nnc_heap_ptr entity) {
    nnc_str name = ((nnc_namespace_statement*)entity)->var->name;
    if (nnc_st_has_namespace(table, name)) {
        THROW(NNC_UNHANDLED, sformat("cannot declare namespace \'%s\', "
            "namespace with this name is already declared.", name));
    }
    nnc_st_put_namespace(table, entity);
}

static void nnc_st_put_enumerator(nnc_st* table, nnc_heap_ptr entity) {
    nnc_enumerator* enumerator = (nnc_enumerator*)entity;
    nnc_str name = enumerator->var->name;
    map_put_s(table->enumerators, name, enumerator);
}

static void nnc_st_try_put_enumerator(nnc_st* table, nnc_heap_ptr entity) {
    //todo: make type declarations topmost
    //todo: add more context to table (global, inside namespace, local etc.)
    nnc_enumerator* enumerator = (nnc_enumerator*)entity;
    nnc_str name = enumerator->var->name;
    if (nnc_st_has_var(table, name)) {
        THROW(NNC_UNHANDLED, sformat("cannot declare enumerator \'%s\', "
            "variable with this name is already declared.", name));
    }
    if (nnc_st_has_enumerator(table, name)) {
        const nnc_enumerator* declared = nnc_st_get_entity(table, ST_ENTITY_ENUMERATOR, name);
        THROW(NNC_UNHANDLED, sformat("enumerator \'%s\' is alredy declared in \'%s\'.", 
            name, nnc_type_tostr(declared->in_enum)));
    }
    nnc_st_put_enumerator(table, entity);
}

void nnc_st_put_entity(nnc_st* table, nnc_st_entity_kind kind, nnc_heap_ptr entity) {
    if (entity == NULL) {
        THROW(NNC_UNHANDLED, "nnc_st_put_entity: entity is NULL.");
    }
    switch (kind) {
        case ST_ENTITY_FN:         nnc_st_try_put_fn(table, entity);         break;
        case ST_ENTITY_VAR:        nnc_st_try_put_var(table, entity);        break;
        case ST_ENTITY_TYPE:       nnc_st_try_put_type(table, entity);       break;
        case ST_ENTITY_PARAM:      nnc_st_try_put_param(table, entity);      break;
        case ST_ENTITY_NAMESPACE:  nnc_st_try_put_namespace(table, entity);  break;
        case ST_ENTITY_ENUMERATOR: nnc_st_try_put_enumerator(table, entity); break;
        default: nnc_abort_no_ctx("nnc_st_put_entity: kind is unknown.\n");
    }
}

nnc_type* nnc_st_get_type(const nnc_heap_ptr entity, nnc_st_entity_kind kind) {
    switch (kind) {
        case ST_ENTITY_FN:          return ((nnc_fn_statement*)entity)->var->type;
        case ST_ENTITY_VAR:         return ((nnc_let_statement*)entity)->type;
        case ST_ENTITY_TYPE:        return ((nnc_type*)entity);
        case ST_ENTITY_PARAM:       return ((nnc_fn_param*)entity)->type;
        case ST_ENTITY_ENUMERATOR:  return &u64_type;
        default: nnc_abort_no_ctx("nnc_st_get_type: unknown kind.");
    }
    return NULL;
}