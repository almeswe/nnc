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
    nnc_symbol* sym = nnc_st_get(table, ident->name);
    if (sym == NULL) {
        nnc_deferred_stack_put(table, DEFERRED_IDENT, ident);
        return false;
    }
    ident->ctx = sym->ctx;
    ident->type = sym->type;
    nnc_deferred_stack_pop(ident);
    return true;
}

static nnc_bool nnc_resolve_namespace(nnc_ident* ident, nnc_st* table) {
    /*nnc_heap_ptr entity = nnc_st_get_entity(table, ST_ENTITY_NAMESPACE, ident->name);
    if (entity != NULL) {
        nnc_deferred_stack_pop(ident);
        return true;
    }
    nnc_deferred_stack_put(table, DEFERRED_NAMESPACE, ident);
    */
    return true;
}

/*static nnc_bool nnc_resolve_scope_expr(nnc_binary_expression* expr, nnc_st* table) {
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
}*/

static void nnc_resolve_as_expr(nnc_unary_expression* unary, nnc_st* table) {
    // note: this is just placeholder, `as` is kind of explicit cast
    // which will be resolved later in process of typechecking.
    unary->type = unary->exact.cast.to;
}

static void nnc_resolve_dot_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_UNION && inner->kind != TYPE_STRUCT) {
        THROW(NNC_SEMANTIC, sformat("cannot access member of (non union or struct) \'%s\' type.", nnc_type_tostr(inner)));
    }
    nnc_ident* member = unary->exact.dot.member->exact;
    for (nnc_u64 i = 0; i < inner->exact.struct_or_union.memberc; i++) {
        nnc_struct_member* struct_member = inner->exact.struct_or_union.members[i];
        if (nnc_sequal(struct_member->var->name, member->name)) {
            unary->type = struct_member->type;
            return;
        }
    }
    THROW(NNC_SEMANTIC, sformat("\'%s\' is not member of \'%s\'.", member->name, nnc_type_tostr(inner)));
}

static void nnc_resolve_call_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_FUNCTION) {
        THROW(NNC_SEMANTIC, sformat("cannot call (non function) \'%s\' type.", nnc_type_tostr(inner)));
    }
    unary->type = inner->exact.fn.ret;
}

static void nnc_resolve_scope_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner_type = nnc_expr_get_type(unary->expr);
    if (inner_type->kind != TYPE_NAMESPACE) {
        THROW(NNC_SEMANTIC, sformat("cannot reference (non namespace) \'%s\' type.", nnc_type_tostr(inner_type)));
    }
    nnc_ident* member = unary->exact.scope.member->exact;
    nnc_namespace_statement* np = inner_type->exact.name.space;
    nnc_st* inner_st = NNC_GET_SYMTABLE(np);
    nnc_symbol* sym = nnc_st_get(inner_st, member->name);
    if (sym == NULL) {
        THROW(NNC_SEMANTIC, sformat("\'%s\' is not listed in \'%s\'.", member->name, nnc_type_tostr(inner_type)));
    }
    member->ctx = sym->ctx;
    member->type = sym->type;
    unary->type = member->type;
}

static void nnc_resolve_index_expr(nnc_unary_expression* unary, nnc_st* table) {
    const nnc_type* inner = nnc_expr_get_type(unary->expr);
    if (inner->kind != TYPE_ARRAY && inner->kind != TYPE_POINTER) {
        THROW(NNC_SEMANTIC, sformat("cannot index (non pointer or array) \'%s\' type.", nnc_type_tostr(inner)));
    }
    unary->type = inner->base;
}

static nnc_bool nnc_resolve_unary_expr(nnc_unary_expression* unary, nnc_st* table) {
    typedef void (unary_expr_resolver)(nnc_unary_expression*, nnc_st*);
    static unary_expr_resolver* resolve[] = {
        [UNARY_POSTFIX_AS]    = nnc_resolve_as_expr,
        [UNARY_POSTFIX_DOT]   = nnc_resolve_dot_expr,
        [UNARY_POSTFIX_CALL]  = nnc_resolve_call_expr,
        [UNARY_POSTFIX_SCOPE] = nnc_resolve_scope_expr,
        [UNARY_POSTFIX_INDEX] = nnc_resolve_index_expr
    };
    if (!nnc_resolve_expr(unary->expr, table)) {
        nnc_deferred_stack_put(table, DEFERRED_UNARY_EXPR, unary);
        return false;
    }
    resolve[unary->kind](unary, table);
    nnc_deferred_stack_pop(unary);
    return true;
}

static nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* expr, nnc_st* table) {
    switch (expr->kind) {
        //case BINARY_SCOPE: return nnc_resolve_scope_expr(expr, table);
        default: return true; //nnc_abort_no_ctx("nnc_resolve_binary_expr: unknown kind.");
    }
    return false;
}

nnc_bool nnc_resolve_entity(nnc_deferred_entity* entity) {
    switch (entity->kind) {
        case DEFERRED_EXPR:         return nnc_resolve_expr(entity->exact, entity->context);
        case DEFERRED_IDENT:        return nnc_resolve_ident(entity->exact, entity->context);
        case DEFERRED_NAMESPACE:    return nnc_resolve_namespace(entity->exact, entity->context);
        //case DEFERRED_SCOPE_EXPR:   return nnc_resolve_scope_expr(entity->exact, entity->context);
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