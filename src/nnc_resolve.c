#include "nnc_resolve.h"
#include "nnc_typecheck.h"
#include "nnc_expression.h"

typedef enum _nnc_semantic_ctx {
    SEMANTIC_CTX_NONE,
    SEMANTIC_CTX_UNION,
    SEMANTIC_CTX_STRUCT,
    SEMANTIC_CTX_NAMESPACE
} nnc_semantic_ctx;

typedef struct _nnc_resolve_ctx {
    nnc_st* table;
    nnc_semantic_ctx semantics;
} nnc_resolve_ctx;

#define resolve_ctx_new(st) (nnc_resolve_ctx) { .table=st, .semantics=SEMANTIC_CTX_NONE }

static nnc_bool nnc_resolve_namespace(nnc_ident* ident, nnc_st* table);

static nnc_bool nnc_resolve_ident(nnc_ident* ident, nnc_st* table) {
    nnc_heap_ptr entity = NULL;
    if (ident->semantics == IDENT_NAMESPACE) {
        return nnc_resolve_namespace(ident, table);
    }
    const nnc_st_entity_kind kind[] = {
        ST_ENTITY_FN, ST_ENTITY_VAR, 
        ST_ENTITY_PARAM, ST_ENTITY_ENUMERATOR
    };
    for (nnc_u64 i = 0; i < 4; i++) {
        entity = nnc_st_get_entity(table, kind[i], ident->name);
        if (entity != NULL) {
            //if (entity != NULL && ((nnc_let_statement*)entity)->is_topmost) {
            ident->type = nnc_st_get_type(entity, kind[i]);
            if (kind[i] == ST_ENTITY_ENUMERATOR) {
                ident->semantics = IDENT_ENUMERATOR;
            }
            nnc_deferred_stack_pop(ident);
            return true;
        }
    }
    nnc_deferred_stack_put(table, DEFERRED_IDENT, ident);
    return false;
}

static nnc_bool nnc_resolve_namespace(nnc_ident* ident, nnc_st* table) {
    nnc_heap_ptr entity = nnc_st_get_entity(table, ST_ENTITY_NAMESPACE, ident->name);
    if (entity != NULL) {
        nnc_deferred_stack_pop(ident);
        return true;
    }
    nnc_deferred_stack_put(table, DEFERRED_NAMESPACE, ident);
    return false;
}

static nnc_bool nnc_resolve_dot_expr(nnc_binary_expression* expr, nnc_resolve_ctx ctx) {
    return true;
}

static nnc_bool nnc_resolve_nested_expr(nnc_binary_expression* expr, nnc_st* table) {
    nnc_ident* namespace_ident = expr->lexpr->exact;
    nnc_namespace_statement* namespace_stmt = NULL;
    if (!nnc_resolve_namespace(namespace_ident, table)) {
        nnc_deferred_stack_put(table, DEFERRED_NESTED_EXPR, expr);
        return false;
    }
    namespace_stmt = nnc_st_get_entity(table, ST_ENTITY_NAMESPACE, namespace_ident->name);
    nnc_st* namespace_st = NNC_GET_SYMTABLE(namespace_stmt);
    assert(namespace_st != NULL);
    nnc_st* namespace_st_root = namespace_st->root;
    // temporarly remove parent table to disable
    // upper check for entity, check only in scope of current table
    namespace_st->root = NULL;
    nnc_bool status = nnc_resolve_expr(expr->rexpr, namespace_st);
    if (status) {
        nnc_deferred_stack_pop(expr);
    }
    else {
        nnc_deferred_stack_put(namespace_st, DEFERRED_EXPR, expr->rexpr);
    }
    // return parent table back
    namespace_st->root = namespace_st_root;
    return status;
}

static nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* expr, nnc_st* table) {
    //return true;
    switch (expr->kind) {
        case BINARY_DOT:  return nnc_resolve_dot_expr(expr, resolve_ctx_new(table));
        case BINARY_NEST: return nnc_resolve_nested_expr(expr, table);
        default: nnc_abort_no_ctx("nnc_resolve_binary_expr: unknown kind.");
    }
    return false;
}

/*static nnc_bool nnc_resolve_namespace_expr(nnc_binary_expression* expr, nnc_st* table) {
    //nnc_expression* member = expr;
    //nnc_deferred_meta* meta = nnc_deferred_meta_get(expr);
    //assert(meta != NULL);
    //while (member->kind != EXPR_IDENT) {
    //    member = ((nnc_binary_expression*)(expr->exact))->rexpr;
    //}
    //assert(member->kind == EXPR_IDENT);
    //nnc_ident* ident = member->exact;
    nnc_ident* namespace_ident = expr->lexpr->exact;
    nnc_namespace_statement* namespace_stmt = nnc_st_check_entity(table, 
        ST_ENTITY_NAMESPACE, namespace_ident->name);
    nnc_st* inner = NNC_GET_SYMTABLE(namespace_stmt);
    assert(inner != NULL);
    nnc_st* inner_root = inner->root;
    // temporarly remove parent table to disable
    // upper check for entity, check only in scope of current table
    inner->root = NULL;
    nnc_bool status = nnc_resolve_expr(expr, inner);
    if (status) {
        nnc_deferred_stack_pop(expr);
    }
    // return parent table back
    inner->root = inner_root;
    return status;
}*/

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity) {
    switch (entity->kind) {
        case DEFERRED_EXPR:         return nnc_resolve_expr(entity->exact, entity->context);
        case DEFERRED_IDENT:        return nnc_resolve_ident(entity->exact, entity->context);
        case DEFERRED_NAMESPACE:    return nnc_resolve_namespace(entity->exact, entity->context);
        case DEFERRED_NESTED_EXPR:  return nnc_resolve_nested_expr(entity->exact, entity->context);
        default: nnc_abort_no_ctx("nnc_resolve_entity: kind unknown.");
    }
    return false;
}

nnc_bool nnc_resolve_expr(nnc_expression* expr, nnc_st* table) {
    switch (expr->kind) {
        case EXPR_IDENT:        return nnc_resolve_ident(expr->exact, table);
        case EXPR_BINARY:       return nnc_resolve_binary_expr(expr->exact, table);
        case EXPR_INT_LITERAL:  return true;
        default: nnc_abort_no_ctx("nnc_resolve_expr: kind unknown.");
    }
    return false;
}