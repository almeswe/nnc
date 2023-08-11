#include "nnc_symtable.h"

void nnc_st_init(nnc_st* out_table) {
    out_table->fns         = map_init_with(1);
    out_table->vars        = map_init_with(1);
    out_table->types       = map_init_with(1);
    out_table->params      = map_init_with(1);
    out_table->namespaces  = map_init_with(1);
    out_table->enumerators = map_init_with(1);
    out_table->syms    = map_init_with(8);
}

nnc_bool nnc_st_has(nnc_st* table, const char* key) {
    nnc_bool has_in_map = false;
    for (; table != NULL && !has_in_map; table = table->root) {
        has_in_map |= map_has_s(table->syms, key);
    }
    return has_in_map;
}

nnc_symbol* nnc_st_get(nnc_st* table, const char* key) {
    nnc_symbol* sym = NULL;
    for (; table != NULL; table = table->root) {
        if (map_has_s(table->syms, key)) {
            sym = map_get_s(table->syms, key);
        }
    }
    return sym;
}

void nnc_st_put(nnc_st* table, nnc_symbol* sym) {
    const static char* ctxs[] = {
        [IDENT_DEFAULT]        = "variable",
        [IDENT_FUNCTION]       = "function",
        [IDENT_NAMESPACE]      = "namespace",
        [IDENT_ENUMERATOR]     = "enumerator",
        [IDENT_FUNCTION_PARAM] = "function param"
    };
    if (nnc_st_has(table, sym->name)) {
        nnc_symbol* in_table_sym = nnc_st_get(table, sym->name);
        if (sym->ctx == in_table_sym->ctx) {
            THROW(NNC_SEMANTIC, sformat("%s \'%s\' is already declared.",
                ctxs[in_table_sym->ctx], in_table_sym->name));
        }
        else {
            THROW(NNC_SEMANTIC, sformat("cannot declare %s, %s with name \'%s\' is already declared.", 
                ctxs[sym->ctx], ctxs[in_table_sym->ctx], in_table_sym->name));
        }
    }
    map_put_s(table->syms, sym->name, sym);
}