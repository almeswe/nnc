#include "nnc_symtable.h"

/**
 * @brief String representation of identifier's context. 
 */
const static char* ictx_str[] = {
    [IDENT_GLOBAL]         = "global",
    [IDENT_DEFAULT]        = "local",
    [IDENT_FUNCTION]       = "function",
    [IDENT_NAMESPACE]      = "namespace",
    [IDENT_ENUMERATOR]     = "enumerator",
    [IDENT_FUNCTION_PARAM] = "function param"
};

/**
 * @brief Initializes preallocated `nnc_st` instance.
 * @param out_table Pointer to preallocated instance of `nns_st` for initialization.
 */
void nnc_st_init(nnc_st* out_table) {
    out_table->ctx = ST_CTX_DEFAULT;
    out_table->syms = map_init_with(8);
    out_table->types = map_init_with(8);
    out_table->ref = (struct _nnc_st_ref){0};
}

/**
 * @brief Releases instance of `nnc_st`.
 * @param st Pointer to `nnc_st` instance to be released.
 */
void nnc_st_fini(nnc_st* st) {
    map_fini(st->syms);
    map_fini(st->types);
    for (nnc_u64 i = 0; i < buf_len(st->branches); i++) {
        nnc_st_fini(st->branches[i]);
    }
    buf_free(st->branches);
}

/**
 * @brief Determines if table contains specified symbol.
 * @param st Pointer to `nnc_st` in which to check for symbol.
 * @param key String representation of symbol.
 * @param kind Kind of symbol.
 * @return `true` if symbol inside of specified `st` or it's parents, `false` otherwise.
 */
nnc_bool nnc_st_has(const nnc_st* st, const char* key, nnc_st_sym_kind kind) {
    nnc_bool has_in_map = false;
    for (; st != NULL && !has_in_map; st = st->root) {
        switch (kind) {
            case ST_SYM_TYPE:  has_in_map |= map_has_s(st->types, key); break; 
            case ST_SYM_IDENT: has_in_map |= map_has_s(st->syms, key);  break; 
            default: {
                nnc_abort_no_ctx("nnc_st_has: unknown sym kind.\n");
            }
        }
        // stop from searching if current `st` is namespace
        // this is done for the purpose of isolating scope of namespace.
        if (st->ctx & ST_CTX_NAMESPACE) {
            break;
        }
    }
    return has_in_map;
}

/**
 * @brief Tryes to get symbol from `st` by it's name.
 *  Performs deep search into `st` branches.
 * @param st Pointer to `nnc_st` in which to check for symbol.
 * @param key String representation of symbol.
 * @return In case when no symbol found, NULL is returned.
 */
nnc_static nnc_heap_ptr nnc_st_deep_get(const nnc_st* st, const char* key) {
    //todo: namespaces support?
    if (map_has_s(st->syms, key)) {
        return map_get_s(st->syms, key);
    }
    for (nnc_u64 i = 0; i < buf_len(st->branches); i++) {
        nnc_heap_ptr sym = nnc_st_deep_get(st->branches[i], key);
        if (sym != NULL) {
            return sym;
        }
    }
    return NULL;
}

/**
 * @brief Tryes to get symbol from `st` by it's name.
 * @param st Pointer to `nnc_st` in which to check for symbol.
 * @param key String representation of symbol.
 * @param kind Kind of symbol.
 * @return In case when no symbol found, NULL is returned.
 */
nnc_heap_ptr nnc_st_get(const nnc_st* st, const char* key, nnc_st_sym_kind kind) {
    nnc_heap_ptr sym = NULL;
    for (; st != NULL && sym == NULL; st = st->root) {
        nnc_map* map = NULL;
        switch (kind) {
            case ST_SYM_TYPE:  map = st->types; break;
            case ST_SYM_IDENT: map = st->syms;  break;
            default: {
                nnc_abort_no_ctx("nnc_st_get: unknown sym kind.\n");
            }
        }
        if (map_has_s(map, key)) {
            sym = map_get_s(map, key);
        }
        // stop from searching if current `st` is namespace
        // this is done for the purpose of isolating scope of namespace.
        if (st->ctx == ST_CTX_NAMESPACE) {
            break;
        }
    }
    return sym;
}

/**
 * @brief Tryes to put symbol into specified `st`.
 * @param st Pointer to `nnc_st` in which to perform inserting.
 * @param sym Pointer to `nnc_sym` to be inserted. 
 * @throw `NNC_NAME_ALREADY_DECLARED` in case when symbol with this name is already inserted.
 */
nnc_static void nnc_st_put_sym(nnc_st* st, nnc_sym* sym) {
    nnc_sym* st_sym = NULL;
    if (sym->ictx == IDENT_GLOBAL) {
        st_sym = nnc_st_deep_get(st, sym->name);
    }
    else {
        st_sym = nnc_st_get(st, sym->name, ST_SYM_IDENT);
    }
    if (st_sym != NULL) {
        if (sym->ictx == st_sym->ictx) {
            THROW(NNC_NAME_ALREADY_DECLARED, sformat("%s \'%s\' is already declared.",
                ictx_str[st_sym->ictx], st_sym->name), sym->ctx);
        }
        else {
            THROW(NNC_NAME_ALREADY_DECLARED, sformat("cannot declare %s, %s with name \'%s\' is already declared.", 
                ictx_str[sym->ictx], ictx_str[st_sym->ictx], st_sym->name), sym->ctx);
        }
    }
    map_put_s(st->syms, sym->name, sym);
}

/**
 * @brief Tryes to put type into specified `st`.
 * @param st Pointer to `nnc_st` in which to perform inserting.
 * @param type Pointer to `nnc_type` to be inserted. 
 * @throw `NNC_TYPE_ALREADY_DECLARED` in case when type with this name is already inserted.
 */
nnc_static void nnc_st_put_type(nnc_st* st, nnc_type* type) {
    assert(type->kind == T_ALIAS);
    if (nnc_st_has_type(st, type->repr)) {
        THROW(NNC_TYPE_ALREADY_DECLARED, sformat("type \'%s\' is already declared.", nnc_type_tostr(type)));
    }
    map_put_s(st->types, type->repr, type);
}

/**
 * @brief Tryes to put generic symbol into specified `st`.
 * @param st Pointer to `nnc_st` in which to perform inserting.
 * @param sym Pointer to generic symbol to be inserted. 
 * @param kind Kind of symbol.
 */
void nnc_st_put(nnc_st* st, nnc_heap_ptr sym, nnc_st_sym_kind kind) {
    switch (kind) {
        case ST_SYM_TYPE:  nnc_st_put_type(st, (nnc_type*)sym); break;
        case ST_SYM_IDENT: nnc_st_put_sym(st, (nnc_sym*)sym);   break;
        default: {
            nnc_abort_no_ctx("nnc_st_put: unknown sym kind.\n");
        }
    }
}

/**
 * @brief Determines if `st` or it's parents has some context.
 *  Optionally may return some symtable which has specified context.
 * @param st Pointer to `st` from which to start. 
 * @param t_st Pointer to preallocated variable in which optionally matched symtable can be stored.
 *  To ignore this value, NULL can be passed.
 * @param ctx Symtable's context to search.
 * @return `true` if found symtable with such context, `false` otherwise.
 */
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