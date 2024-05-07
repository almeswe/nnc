#ifndef __NNC_SYMTABLE_H__
#define __NNC_SYMTABLE_H__

#include "nnc_map.h"

#include "nnc_ast.h"
#include "nnc_literal.h"

typedef struct _nnc_type            nnc_type;
typedef struct _nnc_fn_statement    nnc_fn_statement;

typedef enum _nnc_st_sym_kind {
    ST_SYM_TYPE,
    ST_SYM_IDENT
} nnc_st_sym_kind;

#define SCOPE_FN(x)        ((x)->ctx & ST_CTX_FN)
#define SCOPE_GLOBAL(x)    ((x)->ctx & ST_CTX_GLOBAL)
#define SCOPE_NAMESPACE(x) ((x)->ctx & ST_CTX_NAMESPACE)

typedef enum _nnc_st_ctx {
    ST_CTX_DEFAULT   = 0x00000000,
    ST_CTX_GLOBAL    = 0x00000001,
    ST_CTX_FN        = 0x00000010,
    ST_CTX_LOOP      = 0x00000100,
    ST_CTX_NAMESPACE = 0x00001000,
    ST_CTX_IF        = 0x00010000,
    ST_CTX_ELIF      = 0x00100000,
    ST_CTX_ELSE      = 0x01000000
} nnc_st_ctx;

typedef nnc_ident nnc_sym;

typedef struct _nnc_st {
    nnc_st_ctx ctx;
    struct _nnc_st* root;
    struct _nnc_st** branches;
    _map_(nnc_str, nnc_type*) types;
    _map_(nnc_str, nnc_sym*) syms;
    struct _nnc_st_ref {
        nnc_fn_statement* fn;
        nnc_namespace_statement* np;
    } ref;
} nnc_st;

/**
 * @brief Initializes preallocated `nnc_st` instance.
 * @param out_table Pointer to preallocated instance of `nns_st` for initialization.
 */
void nnc_st_init(
    nnc_st* out_st
);

/**
 * @brief Releases instance of `nnc_st`.
 * @param st Pointer to `nnc_st` instance to be released.
 */
void nnc_st_fini(
    nnc_st* st
);

void nnc_st_put(
    nnc_st* st,
    nnc_heap_ptr sym,
    nnc_st_sym_kind kind
);

/**
 * @brief Determines if table contains certain identifier.
 * @param st Pointer to `nnc_st` in which to check for identifier.
 * @param key String representation of symbol.
 * @return `true` if identifier is inside of specified `st`
 *  or it's parents, `false` otherwise.
 */
#define nnc_st_has_sym(st, key) nnc_st_has(st, key, ST_SYM_IDENT)

/**
 * @brief Determines if table contains certain type.
 * @param st Pointer to `nnc_st` in which to check for type.
 * @param key String representation of symbol.
 * @return `true` if type is inside of specified `st`
 *  or it's parents, `false` otherwise.
 */
#define nnc_st_has_type(st, key) nnc_st_has(st, key, ST_SYM_TYPE)

/**
 * @brief Determines if table contains specified symbol.
 * @param st Pointer to `nnc_st` in which to check for symbol.
 * @param key String representation of symbol.
 * @param kind Kind of symbol.
 * @return `true` if symbol inside of specified `st`
 *  or it's parents, `false` otherwise.
 */
nnc_bool nnc_st_has(
    const nnc_st* st,
    const char* key,
    nnc_st_sym_kind kind
);

/**
 * @brief Determines if table contains certain identifier.
 * @param st Pointer to `nnc_st` in which to check for identifier.
 * @param key String representation of symbol.
 * @return `true` if identifier is inside of specified `st`
 *  or it's parents, `false` otherwise.
 */
#define nnc_st_get_sym(st, sym) \
    (nnc_sym*)nnc_st_get(st, sym, ST_SYM_IDENT)

/**
 * @brief Determines if table contains certain type.
 * @param st Pointer to `nnc_st` in which to check for type.
 * @param key String representation of symbol.
 * @return `true` if type is inside of specified `st`
 *  or it's parents, `false` otherwise.
 */
#define nnc_st_get_type(st, type) \
    (nnc_type*)nnc_st_get(st, type, ST_SYM_TYPE)

/**
 * @brief Tryes to get symbol from `st` by it's name.
 * @param st Pointer to `nnc_st` in which to check for symbol.
 * @param key String representation of symbol.
 * @param kind Kind of symbol.
 * @return In case when no symbol found, NULL is returned.
 */
nnc_heap_ptr nnc_st_get(
    const nnc_st* st,
    const char* key,
    nnc_st_sym_kind kind
);

/**
 * @brief Determines if `st` or it's parents has some context.
 *  Optionally may return some symtable which has specified context.
 * @param st Pointer to `st` from which to start. 
 * @param t_st Pointer to preallocated variable in which
 *  optionally matched symtable can be stored.
 *  To ignore this value, NULL can be passed.
 * @param ctx Symtable's context to search.
 * @return `true` if found symtable with such context, `false` otherwise.
 */
nnc_bool nnc_st_has_ctx(
    const nnc_st* st,
    nnc_st** t_st,
    nnc_st_ctx ctx
);

#endif