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
    if (ident->ctx == IDENT_NAMESPACE) {
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
                ident->ctx = IDENT_ENUMERATOR;
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

static nnc_bool nnc_resolve_scope_expr(nnc_binary_expression* expr, nnc_st* table) {
    nnc_ident* namespace_ident = expr->lexpr->exact;
    nnc_namespace_statement* namespace_stmt = NULL;
    if (!nnc_resolve_namespace(namespace_ident, table)) {
        nnc_deferred_stack_put(table, DEFERRED_SCOPE_EXPR, expr);
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

static void nnc_resolve_as_expr(nnc_unary_expression* unary, nnc_st* table) {
    (void)unary, (void)table;
}

//todo: make more robust errors (remove asserts)

static void nnc_resolve_dot_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    assert(inner->kind == TYPE_UNION || inner->kind == TYPE_STRUCT);
    nnc_ident* member = unary->exact.dot.member->exact;
    for (nnc_u64 i = 0; i < inner->exact.struct_or_union.memberc; i++) {
        nnc_struct_member* struct_member = inner->exact.struct_or_union.members[i];
        if (strcmp(struct_member->var->name, member->name) == 0) {
            unary->type = struct_member->type;
            return;
        }
    }
    THROW(NNC_SEMANTIC, sformat("\'%s\' is not member of \'%s\'.", member->name, nnc_type_tostr(inner)));
}

static void nnc_resolve_call_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    assert(inner->kind == TYPE_FUNCTION);
    unary->type = inner->exact.fn.ret;
}

static void nnc_resolve_index_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    assert(inner->kind == TYPE_ARRAY || inner->kind == TYPE_POINTER);
    unary->type = inner->base;
}

static nnc_bool nnc_resolve_unary_expr(nnc_unary_expression* unary, nnc_st* table) {
    typedef void (unary_expr_resolver)(nnc_unary_expression*, nnc_st*);
    static unary_expr_resolver* resolve[] = {
        [UNARY_POSTFIX_AS]    = nnc_resolve_as_expr,
        [UNARY_POSTFIX_DOT]   = nnc_resolve_dot_expr,
        [UNARY_POSTFIX_CALL]  = nnc_resolve_call_expr,
        [UNARY_POSTFIX_INDEX] = nnc_resolve_index_expr
    };
    if (!nnc_resolve_expr(unary->expr, table)) {
        nnc_deferred_stack_put(table, DEFERRED_UNARY_EXPR, unary);
        return false;
    }
    else {
        resolve[unary->kind](unary, table);
        nnc_deferred_stack_pop(unary);
        return true;
    }
}

static nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* expr, nnc_st* table) {
    switch (expr->kind) {
        case BINARY_SCOPE: return nnc_resolve_scope_expr(expr, table);
        default: return true; //nnc_abort_no_ctx("nnc_resolve_binary_expr: unknown kind.");
    }
    return false;
}

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity) {
    switch (entity->kind) {
        case DEFERRED_EXPR:         return nnc_resolve_expr(entity->exact, entity->context);
        case DEFERRED_IDENT:        return nnc_resolve_ident(entity->exact, entity->context);
        case DEFERRED_NAMESPACE:    return nnc_resolve_namespace(entity->exact, entity->context);
        case DEFERRED_SCOPE_EXPR:   return nnc_resolve_scope_expr(entity->exact, entity->context);
        case DEFERRED_UNARY_EXPR:   return nnc_resolve_unary_expr(entity->exact, entity->context);
        default: nnc_abort_no_ctx("nnc_resolve_entity: kind unknown.");
    }
    return false;
}

nnc_bool nnc_resolve_expr(nnc_expression* expr, nnc_st* table) {
    switch (expr->kind) {
        case EXPR_IDENT:        return nnc_resolve_ident(expr->exact, table);
        case EXPR_INT_LITERAL:  return true;
        case EXPR_UNARY:        return nnc_resolve_unary_expr(expr->exact, table);
        case EXPR_BINARY:       return nnc_resolve_binary_expr(expr->exact, table);
        default: nnc_abort_no_ctx("nnc_resolve_expr: kind unknown.");
    }
    return false;
}