#include "nnc_symtable.h"

void nnc_st_init(nnc_st* out_table) {
    out_table->ctx = ST_CTX_DEFAULT;
    out_table->syms = map_init_with(8);
    out_table->types = map_init_with(8);
    out_table->ref = (struct _nnc_st_ref){0};
}

nnc_bool nnc_st_has(const nnc_st* st, const char* key) {
    nnc_bool has_in_map = false;
    for (; st != NULL && !has_in_map; st = st->root) {
        has_in_map |= map_has_s(st->syms, key);
        if (st->ctx & ST_CTX_NAMESPACE) {
            break;
        }
    }
    return has_in_map;
}

nnc_bool nnc_st_has_type(const nnc_st* st, const char* key) {
    nnc_bool has_in_map = false;
    for (; st != NULL && !has_in_map; st = st->root) {
        has_in_map |= map_has_s(st->types, key);
    }
    return has_in_map; 
}

nnc_symbol* nnc_st_get(const nnc_st* st, const char* key) {
    nnc_symbol* sym = NULL;
    for (; st != NULL; st = st->root) {
        if (map_has_s(st->syms, key)) {
            sym = map_get_s(st->syms, key);
        }
    }
    return sym;
}

nnc_type* nnc_st_get_type(const nnc_st* st, const char* key) {
    nnc_type* type = NULL;
    for (; st != NULL; st = st->root) {
        if (map_has_s(st->types, key)) {
            type = map_get_s(st->types, key);
        }
    }
    return type;
}

nnc_symbol* nnc_st_get_below(const nnc_st* st, const char* key) {
    nnc_st temp_st = *st;
    temp_st.root = NULL;
    return nnc_st_get(&temp_st, key);
}

void nnc_st_put(nnc_st* st, nnc_symbol* sym) {
    const static char* ctxs[] = {
        [IDENT_DEFAULT]        = "variable",
        [IDENT_FUNCTION]       = "function",
        [IDENT_NAMESPACE]      = "namespace",
        [IDENT_ENUMERATOR]     = "enumerator",
        [IDENT_FUNCTION_PARAM] = "function param"
    };
    if (nnc_st_has(st, sym->name)) {
        nnc_symbol* in_table_sym = nnc_st_get(st, sym->name);
        if (sym->ictx == in_table_sym->ictx) {
            THROW(NNC_NAME_ALREADY_DECLARED, sformat("%s \'%s\' is already declared.",
                ctxs[in_table_sym->ictx], in_table_sym->name), sym->ctx);
        }
        else {
            THROW(NNC_NAME_ALREADY_DECLARED, sformat("cannot declare %s, %s with name \'%s\' is already declared.", 
                ctxs[sym->ictx], ctxs[in_table_sym->ictx], in_table_sym->name), sym->ctx);
        }
    }
    map_put_s(st->syms, sym->name, sym);
}

void nnc_st_put_type(nnc_st* st, nnc_type* type) {
    assert(type->kind == T_ALIAS);
    if (nnc_st_has_type(st, type->repr)) {
        THROW(NNC_TYPE_ALREADY_DECLARED, sformat("type \'%s\' is already declared.", nnc_type_tostr(type)));
    }
    map_put_s(st->types, type->repr, type);
}

nnc_bool nnc_st_has_ctx(const nnc_st* st, nnc_st** t_st, nnc_st_ctx ctx) {
    nnc_bool has_ctx = false;
    for (; st != NULL && !has_ctx; st = st->root) {
        if (st->ctx == ctx) {
            if (t_st != NULL) {
                *t_st = (nnc_st*)st;
            }
            has_ctx = true;
        }
    } 
    return has_ctx;
}