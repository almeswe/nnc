#include "nnc_resolve.h"
#include "nnc_typecheck.h"
#include "nnc_expression.h"
#include "nnc_3a.h"

/**
 * @brief Resolves & determines size of specified type. (in bytes)
 * @param type Pointer to `nnc_type` instance.
 * @return Size of type. 
 */
nnc_u64 nnc_sizeof(const nnc_type* type) {
    assert(!nnc_incomplete_type(type));
    return nnc_unalias(type)->size;
}

/**
 * @brief Determines if expression can be folded.
 * It also means that expression contains only constant values. 
 * @param expr Pointer to `nnc_expression` instance to be checked.
 * @return `true` if expression can be folded, `false` otherwise.
 */
nnc_static nnc_bool nnc_can_fold_expr(const nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_STR_LITERAL: return false;
        case EXPR_INT_LITERAL: return true;
        case EXPR_CHR_LITERAL: return true;
        case EXPR_DBL_LITERAL: return true;
        case EXPR_IDENT: {
            const nnc_ident* ident = expr->exact;
            switch (ident->ictx) {
                case IDENT_ENUMERATOR: return true;
                default: return false;
            }
        }
        case EXPR_UNARY: {
            const nnc_unary_expression* unary = expr->exact; 
            switch (unary->kind) {
                case UNARY_POSTFIX_DOT:   return false;
                case UNARY_POSTFIX_INDEX: return false;
                default: break;
            }
            return nnc_can_fold_expr(unary->expr);
        }
        case EXPR_BINARY: {
            const nnc_binary_expression* binary = expr->exact;
            if (binary->kind == BINARY_ASSIGN) {
                return false;
            }
            return nnc_can_fold_expr(binary->lexpr) &&
                   nnc_can_fold_expr(binary->rexpr);
        }
        case EXPR_TERNARY: {
            const nnc_ternary_expression* ternary = expr->exact;
            return nnc_can_fold_expr(ternary->cexpr) &&
                   nnc_can_fold_expr(ternary->lexpr) &&
                   nnc_can_fold_expr(ternary->rexpr);
        }
        default: return false;
    }
}

/**
 * @brief Determines if expression associated with address in memory.
 * @param expr Pointer to `nnc_expression` instance to be checked.
 * @return `true` if expression associated with address in memory, `false` otherwise.
 */
nnc_static nnc_bool nnc_can_locate_expr(const nnc_expression* expr) {
    switch (expr->kind) {
        case EXPR_STR_LITERAL: return true;
        case EXPR_INT_LITERAL: return false;
        case EXPR_CHR_LITERAL: return false;
        case EXPR_DBL_LITERAL: return false;
        case EXPR_IDENT: {
            const nnc_ident* ident = expr->exact;
            switch (ident->ictx) {
                case IDENT_GLOBAL:         return true;
                case IDENT_DEFAULT:        return true;
                case IDENT_FUNCTION:       return true;
                case IDENT_FUNCTION_PARAM: return true;
                default: return false;
            }
        }
        case EXPR_UNARY: {
            const nnc_unary_expression* unary = expr->exact; 
            switch (unary->kind) {
                case UNARY_CAST:          return nnc_can_locate_expr(unary->expr);  
                case UNARY_DEREF:         return true;
                case UNARY_POSTFIX_DOT:   return true;
                case UNARY_POSTFIX_INDEX: return true;
                case UNARY_POSTFIX_AS:    return nnc_can_locate_expr(unary->expr);
                default: return false;
            }
        }
        case EXPR_BINARY: {
            const nnc_binary_expression* binary = expr->exact;
            switch (binary->kind) {
                //case BINARY_ASSIGN: return nnc_can_locate_expr(binary->lexpr);
                default: return false;
            }
        }
        case EXPR_TERNARY: return false;
        default: return false;
    }
}

/**
 * @brief Tryes to complete type in context of specified symtable.
 * @param type Pointer to `nnc_type` instance to be completed.
 * @param st Pointer to `nnc_st` instance.
 * @param ctx Pointer to type context.
 * @throw `NNC_SEMANTIC` in case when `type` is not listed in `st`. 
 */
nnc_static void nnc_complete_type(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    while (type->kind == T_ARRAY ||
           type->kind == T_POINTER) {
        type = type->base;
    }
    if (type->kind != T_INCOMPLETE) {
        return;
    }
    nnc_type* st_type = NULL;
    if (type->repr != NULL) {
        st_type = nnc_st_get_type(st, type->repr);
        if (st_type != NULL) {
            *type = *st_type;
            nnc_complete_type(type->base, st, ctx);
            return;
        }
    }
    assert(ctx != NULL);
    THROW(NNC_SEMANTIC, sformat("incomplete type `%s` met.", type->repr), *ctx);
}

/**
 * @brief Tryes to complete type in context of specified symtable.
 * @param type_expr Expression which contains type and context.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_complete_type_expr(nnc_type_expression* type_expr, nnc_st* st) {
    nnc_complete_type(type_expr->type, st, &type_expr->ctx);
}

/**
 * @brief Resolves condition expression.
 * @param expr Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolve_condition_expr(nnc_expression* expr, nnc_st* st) {
    nnc_resolve_expr(expr, st);
    const nnc_type* t_expr = nnc_expr_get_type(expr); 
    if (!nnc_integral_type(t_expr)) {
        const nnc_ctx* expr_ctx = nnc_expr_get_ctx(expr);
        THROW(NNC_SEMANTIC, "condition must have integral type.", *expr_ctx);
    }
}

/**
 * @brief Determines if struct or union type has circular dependency inside it's members.
 * Example:
 *      type struct {              type struct {      type struct {
 *         a: a_t;         or         b: b_t;            a: a_t;        etc.
 *      } as a_t;                  } as a_t;          } as b_t;
 *
 * @param inside String representation of a type with which we are searching circular dependencies.
 * @param to Type which may have circular dependencies. (check for them within this type)
 * @param st Pointer to `nnc_st` instance.
 * @return `true` if struct or union has circular dependency, `false` otherwise.
 */
nnc_static nnc_bool nnc_struct_has_circular_dep(const nnc_str inside, const nnc_type* to, nnc_st* st) {
    assert(to->kind == T_STRUCT || to->kind == T_UNION);
    const struct _nnc_struct_or_union_type* exact = &to->exact.struct_or_union;
    // iterate through each memeber of `to` type,  
    // and perform circular dependency check for each member
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        // complete member's type basic information
        // `nnc_resolve_type` function here may cause
        // stack overflow in some cases
        nnc_complete_type_expr(m->texpr, st);
        // get pure type in context of an array type
        nnc_type* m_ref = m->texpr->type;
        while (m_ref->kind == T_ARRAY) {
            m_ref = m_ref->base;
        }
        // if type is alias, we can compare it's name with `inside` criteria.
        if (m_ref->kind == T_ALIAS) {
            // if same type detected, circular dependency met 
            if (nnc_strcmp(inside, m_ref->repr)) {
                return true;
            }
            // check circular dependency for next member,
            // but only if this member is struct or union. 
            if (nnc_struct_or_union_type(m_ref->base)) {
                if (nnc_struct_has_circular_dep(inside, m_ref->base, st)) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief Resolves specified type in specified context.
 * This includes type completion and size calculation.
 * @param type Pointer to `nnc_type` instance.
 * @param st Pointer to `nnc_st` instance.
 * @param ctx Pointer to type context.
 * @return `true` if process of resolving succeeded, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_type(nnc_type* type, nnc_st* st, const nnc_ctx* ctx);

/**
 * @brief Resolves type node expression.
 * @param type_expr Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @return `true` if type expression resolved, otherwise `false`.
 */
nnc_static nnc_bool nnc_resolve_type_expr(nnc_type_expression* type_expr, nnc_st* st);

/**
 * @brief Resolves aliased union. This is the same as default
 * type resolving (`nnc_resolve_union`), but here circular dependency check is included.
 * @param alias Pointer to `nnc_type` which is alias to union type.
 * @param ref_type Pointer to `nnc_type` which is actual unaliased union type.
 * @param st Pointer to `nns_st` instance.
 * @param ctx Pointer to type context.
 * @return `true` if process of resolving succeeded, `false` otherwise.
 */
#define nnc_resolve_aliased_union(...) nnc_resolve_aliased_struct(__VA_ARGS__)

/**
 * @brief Resolves aliased struct. This is the same as default
 * type resolving (`nnc_resolve_struct`), but here circular dependency check is included.
 * @param alias Pointer to `nnc_type` which is alias to struct type.
 * @param ref_type Pointer to `nnc_type` which is actual unaliased struct type.
 * @param st Pointer to `nns_st` instance.
 * @param ctx Pointer to type context.
 * @return `true` if process of resolving succeeded, `false` otherwise.
 * @throw `NNC_SEMANTIC` in case when circular dependency met.
 */
nnc_static nnc_bool nnc_resolve_aliased_struct(const nnc_type* alias, nnc_type* ref_type, nnc_st* st, const nnc_ctx* ctx) {
    const nnc_str a_name = alias->repr;
    struct _nnc_struct_or_union_type* exact = &ref_type->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        // complete member's type basic information
        // `nnc_resolve_type` function here may cause
        // stack overflow in some cases
        nnc_complete_type_expr(m->texpr, st);
        // then we need to get pure base type of `m->type`,
        // because size of alias and array types will be determined
        // based on `->base` type, we need to be sure that this `->base` type is not the same 
        // as `alias` type. Otherwise it will cause circular dependency => crash the compiler with stack overflow
        nnc_type* m_ref = m->texpr->type;
        while (m_ref->kind == T_ALIAS ||
               m_ref->kind == T_ARRAY) {
            m_ref = m_ref->base;
        }
        // if pure type is `T_STRUCT` or `T_UNION`, 
        // check it for circular dependency
        if (nnc_struct_or_union_type(m_ref)) {
            if (nnc_struct_has_circular_dep(a_name, m_ref, st)) {
                THROW(NNC_SEMANTIC, sformat("circular dependency met when expanding node "
                    "inside `%s` at `%s::%s`.", a_name, a_name, m->var->name), m->var->ctx);
            }
        }
    }
    // this made because union type is resolved
    // by this function too (`nnc_resolve_aliased_struct`), so to resolve type further
    // depending on it's type (struct or union), calling generic `nnc_resolve_type` here.
    return nnc_resolve_type(ref_type, st, ctx);
}

/**
 * @brief Resolves aliased type. Actually resolves only struct and union, by 
 * calling appropriate `nnc_resolve_aliased_*` function.
 * @param alias Pointer to `nnc_type` alias type.
 * @param st Pointer to `nns_st` instance.
 * @return `true` if process of resolving succeeded, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_aliased_type(nnc_type* alias, nnc_st* st, const nnc_ctx* ctx) {
    nnc_type* ref_type = nnc_unalias(alias);
    switch (ref_type->kind) {
        case T_UNION:  return nnc_resolve_aliased_union(alias, ref_type, st, ctx); 
        case T_STRUCT: return nnc_resolve_aliased_struct(alias, ref_type, st, ctx);
        default: {
            return nnc_resolve_type(alias->base, st, ctx);
        }
    }
}

/**
 * @brief Determines if specified type need size 
 * resolving or not, based on it's kind.
 * @param type Pointer to `nnc_type` instance to be checked.
 * @return `true` if type needs size resolving, `false` otherwise.
 */
nnc_static nnc_bool nnc_type_needs_size_resolve(const nnc_type* type) {
    return type->kind == T_ALIAS  ||
           type->kind == T_STRUCT ||
           type->kind == T_UNION  ||
           type->kind == T_ARRAY;
}

/**
 * @brief Resolves enumerator. 
 * @param enumerator Enumerator to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @return `true` if enumerator resolved, `false` otherwise.
 * @throw `NNC_WRONG_ENUMERATOR_INITIALIZER` in case when enumerator's initializer expression cannot be folded.
 *          it means that value of this expression cannot be determined at compile-time.
 *        `NNC_SEMANTIC` in case when type of initializer expression is not integral.
 */
nnc_static nnc_bool nnc_resolve_enumerator(nnc_enumerator* enumerator, nnc_st* st) {
    //todo: set enumerator's type according to init expression
    if (enumerator->init == NULL) {
        nnc_abort("enumerator must be initialized.", &enumerator->var->ctx);
    }
    nnc_resolve_expr(enumerator->init, st);
    const nnc_ctx* init_expr_ctx = nnc_expr_get_ctx(enumerator->init); 
    if (!nnc_can_fold_expr(enumerator->init)) {
        THROW(NNC_WRONG_ENUMERATOR_INITIALIZER, "enumerator initializer "
            "must be constant expression.", *init_expr_ctx);
    }
    const nnc_type* t_init = nnc_expr_get_type(enumerator->init);
    if (!nnc_integral_type(t_init)) {
        THROW(NNC_SEMANTIC, "enumerator initializer "
            "must be of integral type.", *init_expr_ctx);
    }
    enumerator->var->type = nnc_expr_infer_type(enumerator->init, st);
    enumerator->init_const.d = nnc_evald(enumerator->init, st);
    return true;
}

/**
 * @brief Resolves enum type declaratin.
 * @param type Enum type to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @param ctx Pointer to type context.
 * @return `true` if enum resolved, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_enum(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    /*
    nnc_bool resolved = true;
    for (nnc_u64 i = 0; i < type->exact.enumeration.memberc; i++) {
        nnc_enumerator* enumerator = type->exact.enumeration.members[i];
        resolved &= nnc_resolve_enumerator(enumerator, st);
    }
    return resolved;
    */
    return true;
}

/**
 * @brief Resolves alias type.
 * @param type Alias type to be resolved.
 * @param st Pointer to `nnc_st` instance. 
 * @param ctx Pointer to type context.
 * @return `true` if alias type is resolved, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_alias(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    return nnc_resolve_aliased_type(type, st, ctx);
}

/**
 * @brief Resolves array type.
 * @param type Array type to be resolved.
 * @param st Pointer to `nnc_st` instance. 
 * @param ctx Pointer to type context.
 * @return `true` if alias type is resolved, `false` otherwise.
 * @throw `NNC_SEMANTIC` in case when array's index is not constant expression,
 *          so it can't be determined in compile-time.
 *        `NNC_SEMANTIC` in case when array's index type is not integral type.
 */
nnc_static nnc_bool nnc_resolve_array(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    nnc_expression* dim = type->exact.array.dim;
    const nnc_ctx* dim_ctx = nnc_expr_get_ctx(dim);
    nnc_resolve_expr(dim, st);
    nnc_resolve_type(type->base, st, ctx);
    nnc_type* t_dim = nnc_expr_get_type(dim);
    nnc_complete_type(t_dim, st, dim_ctx);
    if (!nnc_can_fold_expr(dim)) {
        THROW(NNC_SEMANTIC, "array dimension value must be constant.", *dim_ctx);
    }
    if (!nnc_integral_type(t_dim)) {
        THROW(NNC_SEMANTIC, "array dimension must be of integral type.", *dim_ctx);
    }
    type->size = nnc_evald(dim, st) * nnc_sizeof(type->base);
    return true;
}

/**
 * @brief Resolves union type.
 * @param type Union type to be resolved.
 * @param st Pointer to `nnc_st` instance. 
 * @param ctx Pointer to type context.
 * @return `true` if union resolved, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_union(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    type->size = 0;
    struct _nnc_struct_or_union_type* exact = &type->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        nnc_struct_member* m = exact->members[i];
        nnc_complete_type_expr(m->texpr, st);
        if (nnc_type_needs_size_resolve(m->texpr->type)) {
            nnc_resolve_type_expr(m->texpr, st);
        }
        type->size = nnc_max(type->size, nnc_sizeof(m->texpr->type));
    }
    return true;
}

/**
 * @brief Resolves struct type.
 * @param type Struct type to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @param ctx Pointer to type context.
 * @return `true` if struct resolved, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_struct(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    type->size = 0;
    struct _nnc_struct_or_union_type* exact = &type->exact.struct_or_union;
    for (nnc_u64 i = 0; i < exact->memberc; i++) {
        TRY {
            nnc_struct_member* m = exact->members[i];
            nnc_complete_type_expr(m->texpr, st);
            if (nnc_type_needs_size_resolve(m->texpr->type)) {
                nnc_resolve_type_expr(m->texpr, st);
            }
            type->size += nnc_sizeof(m->texpr->type);
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(&CATCHED.where);
        }
    }
    return true;
}

/**
 * @brief Resolves pointer type.
 * @param type Pointer type to be resolved.
 * @param st Pointer to `nnc_st` instance. 
 * @param ctx Pointer to type context.
 * @return `true` if pointer type resolved, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_ptr(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    return nnc_resolve_type(type->base, st, ctx);
}

/**
 * @brief Resolves function type.
 * @param type Function type to be resolved.
 * @param st Pointer to `nnc_st` instance. 
 * @param ctx Pointer to type context.
 * @return `true` if function type resolved, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_fn(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    nnc_bool is_resolved = true;
    for (nnc_u64 i = 0; i < type->exact.fn.paramc && is_resolved; i++) {
        is_resolved &= nnc_resolve_type_expr(type->exact.fn.params[i], st);
    }
    if (type->exact.fn.ret->type->kind == T_VOID) {
        return is_resolved;
    }
    return is_resolved && nnc_resolve_type_expr(type->exact.fn.ret, st);
}

/**
 * @brief Resolves specified type in specified context.
 * This includes type completion and size calculation.
 * @param type Pointer to `nnc_type` instance.
 * @param st Pointer to `nnc_st` instance.
 * @param ctx Pointer to type context.
 * @return `true` if process of resolving succeeded, `false` otherwise.
 */
nnc_static nnc_bool nnc_resolve_type(nnc_type* type, nnc_st* st, const nnc_ctx* ctx) {
    nnc_complete_type(type, st, ctx);
    if (nnc_primitive_type(type) || 
        nnc_namespace_type(type)) {
        return true;
    }
    switch (type->kind) {
        case T_ENUM:     return nnc_resolve_enum(type, st, ctx);
        case T_ALIAS:    return nnc_resolve_alias(type, st, ctx);
        case T_ARRAY:    return nnc_resolve_array(type, st, ctx);
        case T_UNION:    return nnc_resolve_union(type, st, ctx);
        case T_STRUCT:   return nnc_resolve_struct(type, st, ctx);
        case T_POINTER:  return nnc_resolve_ptr(type, st, ctx);
        case T_FUNCTION: return nnc_resolve_fn(type, st, ctx);
        default: break;
    }
    return false;
}

/**
 * @brief Resolves integral literal.
 * @param literal Literal to be resolved.
 * @return `true`.
 */
nnc_static nnc_bool nnc_resolve_int_literal(nnc_int_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_I8:  literal->type = &i8_type;  break;
        case SUFFIX_U8:  literal->type = &u8_type;  break;
        case SUFFIX_I16: literal->type = &i16_type; break;
        case SUFFIX_U16: literal->type = &u16_type; break;
        case SUFFIX_I32: literal->type = &i32_type; break;
        case SUFFIX_U32: literal->type = &u32_type; break;
        case SUFFIX_I64: literal->type = &i64_type; break;
        case SUFFIX_U64: literal->type = &u64_type; break;
        default: {
            nnc_abort("nnc_resolve_int_literal: unknown suffix.\n", &literal->ctx);
        }
    }
    return true;
}

/**
 * @brief Resolves floating point literal.
 * @param literal Literal to be resolved.
 * @return `true`.
 */
nnc_static nnc_bool nnc_resolve_dbl_literal(nnc_dbl_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_F32: literal->type = &f32_type; break;
        case SUFFIX_F64: literal->type = &f64_type; break;
        default: {
            nnc_abort("nnc_resolve_dbl_literal: unknown suffix.\n", &literal->ctx);
        }
    }
    return true;
}

/**
 * @brief Resolves ASCII character literal.
 * @param literal Literal to be resolved.
 * @return `true`.
 */
nnc_static nnc_bool nnc_resolve_chr_literal(nnc_chr_literal* literal) {
    literal->type = &u8_type;
    return true;
}

/**
 * @brief Resolves ASCII string literal.
 * @param literal Literal to be resolved.
 * @return `true`.
 */
nnc_static nnc_bool nnc_resolve_str_literal(nnc_str_literal* literal) {
    literal->type = nnc_ptr_type_new(&u8_type);
    return true; 
}

/**
 * @brief Resolves nesting name. (np1::np2::name etc.)
 * @param nesting Nesting to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @return Pointer to symtable which is local to member which is accessed via netsted name.
 * @throw `NNC_SEMANTIC` in case when nest name is not found.
 *        `NNC_SEMANTIC` in case when nest has non namespace type.
 */
nnc_static nnc_st* nnc_resolve_nesting(nnc_nesting* nesting, nnc_st* st) {
    if (nesting == NULL) {
        return st;
    }
    nnc_ident* nest = nesting->nest;
    nnc_sym* sym = nnc_st_get_sym(st, nest->name);
    if (sym == NULL) {
        THROW(NNC_SEMANTIC, sformat("cannot resolve nesting `%s`.", nest->name), nest->ctx);
    }
    if (!nnc_namespace_type(sym->type)) {
        THROW(NNC_SEMANTIC, sformat("nesting must be namespace, not `%s`.", 
            nnc_type_tostr(sym->type)), nest->ctx);
    }
    *nest = *sym;
    st = NNC_GET_SYMTABLE(sym->type->exact.name.space);
    return nnc_resolve_nesting(nesting->next, st);
}

/**
 * @brief Appends implicit nesting to existing.
 * @param ident Pointer to `nnc_ident` to which nesting must be appended.
 * @param nesting Nesting itself to be appended.
 */
nnc_static void nnc_add_imp_nesting(nnc_ident* ident, nnc_nesting* nesting) {
    if (ident->nesting != NULL) {
        nnc_nesting* root = nesting;
        while (root->next != NULL) {
            root = root->next;
        }
        root->next = ident->nesting;
    }
    ident->nesting = nesting;
}

/**
 * @brief Retrieves implicit nesting by traversing the symtable backwards.
 * @param st Pointer to `nns_st`.
 * @return `nnc_nesting` instance from traversed symtable or NULL.
 */
nnc_static nnc_nesting* nnc_get_imp_nesting(nnc_st* st) {
    nnc_st** path = NULL;
    while (st != NULL && st->root != NULL) {
        if (st->ctx == ST_CTX_NAMESPACE) {
            buf_add(path, st);
        }
        st = st->root;
    }
    if (buf_len(path) == 0) {
        return NULL;
    }
    nnc_nesting* root = NULL;
    nnc_nesting* nesting = NULL;
    for (nnc_i64 i = (nnc_i64)buf_len(path) - 1; i >= 0; i--) {
        if (nesting == NULL) {
            nesting = nnc_new(nnc_nesting);
            root = nesting;    
        }
        else {
            nesting->next = nnc_new(nnc_nesting);
            nesting = nesting->next;
        }
        nesting->nest = path[i]->ref.np->var;
    }
    return root;
}

/**
 * @brief Resolve identifier.
 * @param ident Identifier to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @return `true` if identifier resolved, `false` otherwise.
 * @throw `NNC_SEMANTIC` in case when identifier is not listed in current context.
 */
nnc_static nnc_bool nnc_resolve_ident(nnc_ident* ident, nnc_st* st) {
    // get explicit nesting for resolving
    // need to save it before it can be modified by `nnc_add_imp_nesting`
    nnc_nesting* exp_nesting = ident->nesting;
    // get implicit nesting
    nnc_nesting* imp_nesting = nnc_get_imp_nesting(st);
    if (imp_nesting != NULL) {
        // append it to existing nesting
        nnc_add_imp_nesting(ident, imp_nesting);    
    }
    if (exp_nesting != NULL) {
        // `st_to_resolve` is original `st` before
        // any implicit nesting is added
        st = nnc_resolve_nesting(exp_nesting, st);
    }
    nnc_sym* sym = nnc_st_get_sym(st, ident->name);
    if (sym == NULL) {
        THROW(NNC_SEMANTIC, sformat("undeclared identifier `%s` met.", ident->name), ident->ctx);
    }
    ident->ictx = sym->ictx;
    ident->type = sym->type;
    nnc_resolve_type(ident->type, st, &ident->ctx);
    if (ident->ictx == IDENT_ENUMERATOR) {
        ident->type = &i64_type;
        ident->refs = sym->refs;
        nnc_resolve_enumerator(sym->refs.enumerator, st);
    }
    return true;
}

/**
 * @brief Resolves unary reference expression (&<expr>).
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_REF_EXPR` in case when referenced expression is not locatable.
 *  So basically address cannot be retrieved.
 */
nnc_static void nnc_resolve_ref_expr(nnc_unary_expression* unary, nnc_st* st) {
    if (!nnc_can_locate_expr(unary->expr)) {
        THROW(NNC_CANNOT_RESOLVE_REF_EXPR, "cannot reference non locatable expression.", unary->ctx);
    }
    nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    unary->type = nnc_ptr_type_new(t_expr);
}

/**
 * @brief Resolves unary not expression (!<expr>).
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_NOT_EXPR` in case when referenced expression is not locatable.
 *  So basically address cannot be retrieved.
 */
nnc_static void nnc_resolve_not_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_integral_type(t_expr) && !nnc_arr_or_ptr_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_NOT_EXPR, "cannot use logical not for this expression.", unary->ctx);
    }
    unary->type = &i8_type;
}

/**
 * @brief Resolves unary cast expression ((<type>)<expr>).
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when cannot explicitly cast type of expression to specified type.
 */
nnc_static void nnc_resolve_cast_expr(nnc_unary_expression* unary, nnc_st* st) {
    nnc_resolve_type_expr(unary->exact.cast.to, st);
    const nnc_type* t_cast = unary->exact.cast.to->type;
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_can_exp_cast(t_expr, t_cast)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast `%s` to `%s`.", 
            nnc_type_tostr(t_expr), nnc_type_tostr(t_cast)), unary->ctx);
    }
    unary->type = unary->exact.cast.to->type;
}

/**
 * @brief Resolves unary plus expression (+<expr>). 
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_PLUS_EXPR` in case when applied to non numeric expression. 
 */
nnc_static void nnc_resolve_plus_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_numeric_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_PLUS_EXPR, "expression must have numeric type.", unary->ctx);
    }
    unary->type = nnc_expr_get_type(unary->expr);
}

/**
 * @brief Resolves unary minus expression (-<expr>). 
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_MINUS_EXPR` in case when applied to non numeric expression. 
 */
nnc_static void nnc_resolve_minus_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_numeric_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_MINUS_EXPR, "expression must have numeric type.", unary->ctx);
    }
    unary->type = nnc_expr_get_type(unary->expr);
}

/**
 * @brief Resolves unary deference expression (*<expr>). 
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_DEREF_EXPR` in case when applied to non array or pointer. 
 */
nnc_static void nnc_resolve_deref_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_arr_or_ptr_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_DEREF_EXPR, sformat("cannot dereference (non array or pointer) "
            "`%s` type.", nnc_type_tostr(t_expr)), unary->ctx);
    }
    unary->type = t_expr->base;
}

/**
 * @brief Resolves unary sizeof expression (sizeof(<type>)). 
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolve_sizeof_expr(nnc_unary_expression* unary, nnc_st* st) {
    nnc_resolve_type_expr(unary->exact.size.of, st);
    assert(unary->expr == NULL);
    unary->type = &u64_type;
}

/**
 * @brief Resolves unary lengthof expression (lengthof <expr>). 
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolve_lengthof_expr(nnc_unary_expression* unary, nnc_st* st) {
    unary->type = &u64_type;
}

/**
 * @brief Resolves unary bitwise not expression (~<expr>).
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_BW_NOT_EXPR` in case when applied to integral expression.
 */
nnc_static void nnc_resolve_bitwise_not_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_integral_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_BW_NOT_EXPR, "expression must have integral type.", unary->ctx);
    }
    unary->type = nnc_expr_get_type(unary->expr);
}

/**
 * @brief Resolves unary `as` expression (<expr> as <type>).
 * It differs from default `cast` with independent type declaration order.
 * So if you declare type after `as` expression is used, it will be ok.
 * `cast` expression in this case will not work. (syntax error)
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @return `NNC_SEMANTIC` in case when cannot explicitly cast expression type to `as` type. 
 */
nnc_static void nnc_resolve_as_expr(nnc_unary_expression* unary, nnc_st* st) {
    nnc_resolve_type_expr(unary->exact.cast.to, st);
    const nnc_type* t_as = unary->exact.cast.to->type;
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_can_exp_cast(t_expr, t_as)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast `%s` to `%s`.", 
            nnc_type_tostr(t_expr), nnc_type_tostr(t_as)), unary->ctx);
    }
    else {
        //todo: check for const expression + cast to expansion type
    }
    unary->type = unary->exact.cast.to->type;
}

/**
 * @brief Resolves unary dot expression (<expr>.<ident>).
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_DOT_EXPR` in case when expression is not struct or union.
 *        `NNC_CANNOT_RESOLVE_DOT_EXPR` in case when accessed member is not listed in struct or union.
 */
nnc_static void nnc_resolve_dot_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_struct_or_union_type(t_expr)) {
        // check if trying to access member 
        // of pointer to struct or union
        if (nnc_ptr_type(t_expr)) {
            t_expr = t_expr->base;
        }
        if (!nnc_struct_or_union_type(t_expr)) {
            THROW(NNC_CANNOT_RESOLVE_DOT_EXPR, sformat("cannot access member of (non union or struct) "
                "`%s` type.", nnc_type_tostr(t_expr)), unary->ctx);
        }
    }
    t_expr = nnc_unalias(t_expr);
    nnc_ident* m = unary->exact.dot.member->exact;
    for (nnc_u64 i = 0; i < t_expr->exact.struct_or_union.memberc; i++) {
        nnc_struct_member* s_m = t_expr->exact.struct_or_union.members[i];
        if (nnc_strcmp(s_m->var->name, m->name)) {
            unary->type = s_m->texpr->type;
            m->type = unary->type;
            return;
        }
    }
    THROW(NNC_CANNOT_RESOLVE_DOT_EXPR, sformat("`%s` is not member of " 
        "`%s`.", m->name, nnc_type_tostr(t_expr)), unary->ctx);
}

/**
 * @brief Resolves call expression. (<expr>({<expr>}+))
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_CALL_EXPR` in case when applied to non function expression.
 *        `NNC_CANNOT_RESOLVE_CALL_EXPR` in case when amount of needed arguments is not the same as specified.
 *        `NNC_CANNOT_RESOLVE_CALL_EXPR` in case when type of some argument is not the same with type from function declaration.
 */
nnc_static void nnc_resolve_call_expr(nnc_unary_expression* unary, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_fn_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_CALL_EXPR, sformat("cannot call (non function) "
            "`%s` type.", nnc_type_tostr(t_expr)), unary->ctx);
    }
    const struct _nnc_fn_type* f_type = &t_expr->exact.fn;
    const struct _nnc_unary_postfix_call* c_expr = &unary->exact.call;
    // check amount of arguments needed, and amount specified.
    if (f_type->paramc != c_expr->argc) {
        THROW(NNC_CANNOT_RESOLVE_CALL_EXPR, sformat("function has %lu argument(s), "
            "but %lu was(were) given.", f_type->paramc, c_expr->argc), unary->ctx);
    }
    // then resolve all arguments, and check their types
    for (nnc_u64 i = 0; i < c_expr->argc; i++) {
        nnc_resolve_expr(c_expr->args[i], st);
        const nnc_type* t_param = f_type->params[i]->type;
        const nnc_type* t_arg = nnc_expr_get_type(c_expr->args[i]);  
        if (!nnc_can_imp_cast_assign(t_arg, t_param)) {
            THROW(NNC_CANNOT_RESOLVE_CALL_EXPR, sformat("cannot use argument of type `%s`, "
                "when function has `%s`.", nnc_type_tostr(t_arg), nnc_type_tostr(t_param)), unary->ctx);
        }
    }
    assert(f_type->ret != NULL);
    nnc_resolve_type(f_type->ret->type, st, &unary->ctx);
    unary->type = f_type->ret->type;
}

/**
 * @brief Resolves index expression. (<expr>[<expr>])
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_INDEX_EXPR` in case when trying to index non array or pointer expression.
 *        `NNC_CANNOT_RESOLVE_INDEX_EXPR` in case when index expression is not integral.
 */
nnc_static void nnc_resolve_index_expr(nnc_unary_expression* unary, nnc_st* st) {
    nnc_resolve_expr(unary->exact.index.expr, st);
    const nnc_type* t_expr = nnc_expr_get_type(unary->expr);
    if (!nnc_arr_or_ptr_type(t_expr)) {
        THROW(NNC_CANNOT_RESOLVE_INDEX_EXPR, sformat("cannot index (non pointer or array) "
            "`%s` type.", nnc_type_tostr(t_expr)), unary->ctx);
    }
    const nnc_type* t_index = nnc_expr_get_type(unary->exact.index.expr);
    if (!nnc_integral_type(t_index)) {
        THROW(NNC_CANNOT_RESOLVE_INDEX_EXPR, sformat("cannot index with non integral "
            "`%s` type.", nnc_type_tostr(t_index)), unary->ctx);
    }
    unary->type = t_expr->base;
}

/**
 * @brief Resolves unary expression.
 * @param unary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static nnc_bool nnc_resolve_unary_expr(nnc_unary_expression* unary, nnc_st* st) {
    if (unary->expr != NULL) {
        nnc_resolve_expr(unary->expr, st);
    }
    switch (unary->kind) {
        case UNARY_REF:           nnc_resolve_ref_expr(unary, st);         break;
        case UNARY_NOT:           nnc_resolve_not_expr(unary, st);         break;
        case UNARY_CAST:          nnc_resolve_cast_expr(unary, st);        break;
        case UNARY_PLUS:          nnc_resolve_plus_expr(unary, st);        break;
        case UNARY_MINUS:         nnc_resolve_minus_expr(unary, st);       break;
        case UNARY_DEREF:         nnc_resolve_deref_expr(unary, st);       break;
        case UNARY_SIZEOF:        nnc_resolve_sizeof_expr(unary, st);      break;
        case UNARY_LENGTHOF:      nnc_resolve_lengthof_expr(unary, st);    break;
        case UNARY_BITWISE_NOT:   nnc_resolve_bitwise_not_expr(unary, st); break;
        case UNARY_POSTFIX_AS:    nnc_resolve_as_expr(unary, st);          break;
        case UNARY_POSTFIX_DOT:   nnc_resolve_dot_expr(unary, st);         break;
        case UNARY_POSTFIX_CALL:  nnc_resolve_call_expr(unary, st);        break;
        case UNARY_POSTFIX_INDEX: nnc_resolve_index_expr(unary, st);       break;
        default: {
            nnc_abort("nnc_resolve_unary_expr: unknown kind.\n", &unary->ctx);
        }
    }
    return true;
}

/**
 * @brief Resolves binary expression type.
 *  Makes call for `nnc_binary_expr_infer_type`, with try-catch block.
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_ASSIGN_EXPR` in case when cannot locate left expression.
 */
nnc_static void nnc_resolve_binary_expr_type(nnc_binary_expression* binary, nnc_st* st) {
    TRY {
        nnc_binary_expr_infer_type(binary, st);
        ETRY;
    }
    CATCH (NNC_SEMANTIC) {
        // make this particular exception catch, for 
        // setting context to the exception, and then rethrow.
        const nnc_ctx* ctx = nnc_expr_get_ctx(binary->rexpr);
        THROW(NNC_SEMANTIC, CATCHED.what, *ctx);
    }
    CATCHALL {
        RETHROW;
    }
}

/**
 * @brief Resolves binary addition expression. (<expr>[`+` | `-`]<expr>)
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_ADD_EXPR` in case when left expression is pointer or array, but right expression is not integral.
 *        `NNC_CANNOT_RESOLVE_ADD_EXPR` in case when right expression is pointer or array, but left expression is not integral.
 *        `NNC_CANNOT_RESOLVE_ADD_EXPR` in case when both expressions have non numeric types.
 */
nnc_static void nnc_resolve_add_expr(nnc_binary_expression* binary, nnc_st* st) {
    nnc_type* t_lexpr = nnc_expr_get_type(binary->lexpr);
    nnc_type* t_rexpr = nnc_expr_get_type(binary->rexpr);
    if (nnc_arr_or_ptr_type(t_lexpr)) {
        if (!nnc_integral_type(t_rexpr)) {
            THROW(NNC_CANNOT_RESOLVE_ADD_EXPR, "right expression "
                "must have integral type.", binary->ctx);
        }
        binary->type = t_lexpr;
        return;
    }
    if (nnc_arr_or_ptr_type(t_rexpr)) {
        if (!nnc_integral_type(t_lexpr)) {
            THROW(NNC_CANNOT_RESOLVE_ADD_EXPR, "left expression "
                "must have integral type.", binary->ctx);
        }
        binary->type = t_rexpr;
        return;
    }
    if (!nnc_numeric_type(t_lexpr) && !nnc_numeric_type(t_rexpr)) {
        THROW(NNC_CANNOT_RESOLVE_ADD_EXPR, "both expressions "
            "must have numeric types.", binary->ctx);
    }
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary multiplication expression. (<expr>[`*` | `/` | `%`]<expr>)
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_SHIFT_EXPR` in case when both expression have non integral or numeric types.
 */
nnc_static void nnc_resolve_mul_expr(nnc_binary_expression* binary, nnc_st* st) {
    nnc_type* t_lexpr = nnc_expr_get_type(binary->lexpr);
    nnc_type* t_rexpr = nnc_expr_get_type(binary->rexpr);
    if (binary->kind == BINARY_MOD) {
        if (!nnc_integral_type(t_lexpr) || !nnc_integral_type(t_rexpr)) {
            THROW(NNC_CANNOT_RESOLVE_MUL_EXPR, "both expressions "
                "must have integral types.", binary->ctx);
        }
    }
    if (!nnc_numeric_type(t_lexpr) && !nnc_numeric_type(t_rexpr)) {
        THROW(NNC_CANNOT_RESOLVE_MUL_EXPR, "both expressions "
            "must have numeric types.", binary->ctx);
    }
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary shift expression. (<expr>[`<<` | `>>`]<expr>)
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_SHIFT_EXPR` in case when one or both expressions are not integral.
 */
nnc_static void nnc_resolve_shift_expr(nnc_binary_expression* binary, nnc_st* st) {
    const nnc_type* t_lexpr = nnc_expr_get_type(binary->lexpr);
    const nnc_type* t_rexpr = nnc_expr_get_type(binary->rexpr);
    if (!nnc_integral_type(t_lexpr) || !nnc_integral_type(t_rexpr)) {
        THROW(NNC_CANNOT_RESOLVE_SHIFT_EXPR, "both expressions "
            "must have integral types.", binary->ctx);
    }
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary relation expression. (<expr>[`||` | `>`  | `<`  | `==` | `!=`]<expr>)
 *                                                    [`&&` | `>=` | `<=` |            ]
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_REL_EXPR` in case when left expression is pointer or array, but right expression is not integral.
 *        `NNC_CANNOT_RESOLVE_REL_EXPR` in case when right expression is pointer or array, but left expression is not integral.
 *        `NNC_CANNOT_RESOLVE_REL_EXPR` in case when both expressions have non numeric types.
 */
nnc_static void nnc_resolve_rel_expr(nnc_binary_expression* binary, nnc_st* st) {
    //todo: pointer arithmetic?
    nnc_type* t_lexpr = nnc_expr_get_type(binary->lexpr);
    nnc_type* t_rexpr = nnc_expr_get_type(binary->rexpr);
    if (nnc_arr_or_ptr_type(t_lexpr)) {
        if (!nnc_integral_type(t_rexpr)) {
            THROW(NNC_CANNOT_RESOLVE_REL_EXPR, "right expression "
                "must have integral type.", binary->ctx);
        }
    }
    else if (nnc_arr_or_ptr_type(t_rexpr)) {
        if (!nnc_integral_type(t_lexpr)) {
            THROW(NNC_CANNOT_RESOLVE_REL_EXPR, "left expression "
                "must have integral type.", binary->ctx);
        }
    }   
    else if (!nnc_numeric_type(t_lexpr) && !nnc_numeric_type(t_rexpr)) {
        THROW(NNC_CANNOT_RESOLVE_REL_EXPR, "both expressions "
            "must have numeric types.", binary->ctx);
    }
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary bitwise expression. (<expr>[`|` | `&` | `^`]<expr>)
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_BITWISE_EXPR` in case when at least one expression has non integral type. 
 */
nnc_static void nnc_resolve_bitwise_expr(nnc_binary_expression* binary, nnc_st* st) {
    const nnc_type* t_lexpr = nnc_expr_get_type(binary->lexpr);
    const nnc_type* t_rexpr = nnc_expr_get_type(binary->rexpr);
    if (!nnc_integral_type(t_lexpr) || !nnc_integral_type(t_rexpr)) {
        THROW(NNC_CANNOT_RESOLVE_BITWISE_EXPR, "both expressions "
            "must have integral types.", binary->ctx);
    }
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary assignment expression. (<expr>`=`<expr>)
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_ASSIGN_EXPR` in case when cannot locate left expression.
 */
nnc_static void nnc_resolve_assign_expr(nnc_binary_expression* binary, nnc_st* st) {
    if (!nnc_can_locate_expr(binary->lexpr)) {
        THROW(NNC_CANNOT_ASSIGN_EXPR, "left expression "
            "must be locatable.", binary->ctx);
    }
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary comma expression. (<expr>`,`<expr>)
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolve_comma_expr(nnc_binary_expression* binary, nnc_st* st) {
    nnc_resolve_binary_expr_type(binary, st);
}

/**
 * @brief Resolves binary expression.
 * @param binary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static nnc_bool nnc_resolve_binary_expr(nnc_binary_expression* binary, nnc_st* st) {
    if (!nnc_resolve_expr(binary->lexpr, st) ||
        !nnc_resolve_expr(binary->rexpr, st)) {
        return false;
    }
    switch (binary->kind) {
        case BINARY_OR:     nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_LT:     nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_GT:     nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_EQ:     nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_ADD:    nnc_resolve_add_expr(binary, st);     break;
        case BINARY_SUB:    nnc_resolve_add_expr(binary, st);     break;
        case BINARY_MUL:    nnc_resolve_mul_expr(binary, st);     break;
        case BINARY_DIV:    nnc_resolve_mul_expr(binary, st);     break;
        case BINARY_MOD:    nnc_resolve_mul_expr(binary, st);     break;
        case BINARY_SHR:    nnc_resolve_shift_expr(binary, st);   break;
        case BINARY_SHL:    nnc_resolve_shift_expr(binary, st);   break;
        case BINARY_NEQ:    nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_AND:    nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_LTE:    nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_GTE:    nnc_resolve_rel_expr(binary, st);     break;
        case BINARY_COMMA:  nnc_resolve_comma_expr(binary, st);   break;
        case BINARY_BW_OR:  nnc_resolve_bitwise_expr(binary, st); break;
        case BINARY_BW_AND: nnc_resolve_bitwise_expr(binary, st); break;
        case BINARY_BW_XOR: nnc_resolve_bitwise_expr(binary, st); break;
        case BINARY_ASSIGN: nnc_resolve_assign_expr(binary, st);  break;
        default: {
            nnc_abort("nnc_resolve_binary_expr: unknown kind.\n", &binary->ctx);
        }
    } 
    return true;
}

/**
 * @brief Resolves ternary expression. (<expr>`?`<expr>`:`<expr>)
 * @param ternary Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_CANNOT_RESOLVE_TERNARY_EXPR` in case when condition expression is not integral.
 */
nnc_static nnc_bool nnc_resolve_ternary_expr(nnc_ternary_expression* ternary, nnc_st* st) {
    nnc_resolve_condition_expr(ternary->cexpr, st);
    nnc_resolve_expr(ternary->lexpr, st);
    nnc_resolve_expr(ternary->rexpr, st);
    nnc_ternary_expr_infer_type(ternary, st);
    return true;
}

/**
 * @brief Resolves type node expression.
 * @param type_expr Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @return `true` if type expression resolved, otherwise `false`.
 */
nnc_static nnc_bool nnc_resolve_type_expr(nnc_type_expression* type_expr, nnc_st* st) {
    if (type_expr->nesting != NULL) {
        // resolving nested name if it was specified
        // and use returned local symtable as current symtable
        // for further type resolving.
        st = nnc_resolve_nesting(type_expr->nesting, st);
    }
    return nnc_resolve_type(type_expr->type, st, &type_expr->ctx);
}

/**
 * @brief Resolves expression.
 * @param expr Expression to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_bool nnc_resolve_expr(nnc_expression* expr, nnc_st* st) {
    if (expr == NULL) {
        return true;
    }
    switch (expr->kind) {
        case EXPR_INT_LITERAL:  return nnc_resolve_int_literal(expr->exact);
        case EXPR_DBL_LITERAL:  return nnc_resolve_dbl_literal(expr->exact);
        case EXPR_CHR_LITERAL:  return nnc_resolve_chr_literal(expr->exact);
        case EXPR_STR_LITERAL:  return nnc_resolve_str_literal(expr->exact);
        case EXPR_IDENT:        return nnc_resolve_ident(expr->exact, st);
        case EXPR_UNARY:        return nnc_resolve_unary_expr(expr->exact, st);
        case EXPR_BINARY:       return nnc_resolve_binary_expr(expr->exact, st);
        case EXPR_TERNARY:      return nnc_resolve_ternary_expr(expr->exact, st);
        default: {
            nnc_abort("nnc_resolve_expr: unknown kind.\n", nnc_expr_get_ctx(expr));
        }
    }
    return false;
}

/**
 * @brief Resolves list of function params.
 * @param params Array of function params to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolve_params(nnc_fn_param** params, nnc_st* st) {
    for (nnc_u64 i = 0; i < buf_len(params); i++) {
        TRY {
            nnc_resolve_type_expr(params[i]->texpr, st);
            params[i]->var->type = params[i]->texpr->type;
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(&CATCHED.where);
        }
    }
}

/**
 * @brief Resolves do-while statement.
 * @param do_stmt Statement to be resolved. 
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when statement is declared outside function context.
 */
nnc_static void nnc_resolve_do_stmt(nnc_do_while_statement* do_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_FN)) {
        THROW(NNC_SEMANTIC, "cannot declare `do` in this context.");
    }
    nnc_resolve_stmt(do_stmt->body, st);
    nnc_resolve_condition_expr(do_stmt->cond, st);
}

/**
 * @brief Resolves function statemnt.
 * @param fn_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when statement not declared in namespace or global scope. 
 */
nnc_static void nnc_resolve_fn_stmt(nnc_fn_statement* fn_stmt, nnc_st* st) {
    if (st->ctx != ST_CTX_GLOBAL &&
        st->ctx != ST_CTX_NAMESPACE) {
        THROW(NNC_SEMANTIC, "cannot declare `fn` in this context.", fn_stmt->var->ctx);
    }
    fn_stmt->var->nesting = nnc_get_imp_nesting(st);
    nnc_resolve_params(fn_stmt->params, st);
    nnc_resolve_type_expr(fn_stmt->ret, st);
    if (fn_stmt->storage != FN_ST_EXTERN) {
        nnc_resolve_stmt(fn_stmt->body, st);
    }
}

/**
 * @brief Resolves if-else statement.
 * @param if_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when statement declared outside function.
 */
nnc_static void nnc_resolve_if_stmt(nnc_if_statement* if_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_FN)) {
        THROW(NNC_SEMANTIC, "cannot declare `if-else` in this context.");
    }
    nnc_resolve_condition_expr(if_stmt->if_br->cond, st);
    nnc_resolve_stmt(if_stmt->if_br->body, st);
    for (nnc_u64 i = 0; i < buf_len(if_stmt->elif_brs); i++) {
        nnc_resolve_condition_expr(if_stmt->elif_brs[i]->cond, st);
        nnc_resolve_stmt(if_stmt->elif_brs[i]->body, st);
    }
    if (if_stmt->else_br != NULL) {
        nnc_resolve_stmt(if_stmt->else_br, st);
    }
}

/**
 * @brief Resolves for statement.
 * @param for_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when statement declared outside function.
 *        `NNC_SEMANTIC` in case when declaring variable in init expression without any body.
 *        (it means that variable will not be used, so this is pointless)
 */
nnc_static void nnc_resolve_for_stmt(nnc_for_statement* for_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_FN)) {
        THROW(NNC_SEMANTIC, "cannot declare `for` in this context.");
    }
    if (for_stmt->init->kind == STMT_LET &&
        for_stmt->body->kind != STMT_COMPOUND) {
        THROW(NNC_SEMANTIC, "cannot declare variable in this context.");
    }
    nnc_resolve_stmt(for_stmt->init, st);
    nnc_st* current = st;
    if (for_stmt->init->kind == STMT_LET) {
        current = NNC_GET_SYMTABLE(for_stmt);
    }
    if (for_stmt->cond->kind == STMT_EXPR) {
        nnc_expression_statement* expr_stmt = for_stmt->cond->exact; 
        nnc_resolve_condition_expr(expr_stmt->expr, current);
    }
    nnc_resolve_stmt(for_stmt->step, current);
    nnc_resolve_stmt(for_stmt->body, st);
}

/**
 * @brief Resolves let statement.
 * @param let_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when initializer expression is not the same as type of declarable variable.
 */
nnc_static void nnc_resolve_let_stmt(nnc_let_statement* let_stmt, nnc_st* st) {
    nnc_resolve_type_expr(let_stmt->texpr, st);
    if (let_stmt->init != NULL) {
        nnc_resolve_expr(let_stmt->init, st);
        const nnc_ctx* init_expr_ctx = nnc_expr_get_ctx(let_stmt->init);
        if (st->ctx == ST_CTX_GLOBAL ||
            st->ctx == ST_CTX_NAMESPACE) {
            if (!nnc_can_fold_expr(let_stmt->init)) {
                THROW(NNC_SEMANTIC, "cannot initialize global variable"
                    " with non-constant expression.", *init_expr_ctx);
            }
        }
        const nnc_type* t_init = nnc_expr_get_type(let_stmt->init);
        if (!nnc_can_imp_cast_assign(t_init, let_stmt->texpr->type)) {
            THROW(NNC_SEMANTIC, sformat("cannot initialize variable "
                "with expression of `%s` type.", nnc_type_tostr(t_init)), *init_expr_ctx);
        }
    }
}

/**
 * @brief Resolves type statement.
 * @param type_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when declared not in global scope.
 */
nnc_static void nnc_resolve_type_stmt(nnc_type_statement* type_stmt, nnc_st* st) {
    if (st->ctx != ST_CTX_GLOBAL &&
        st->ctx != ST_CTX_NAMESPACE) {
        THROW(NNC_SEMANTIC, "cannot declare `type` in this context.");
    }
    const nnc_ctx* ctx = &type_stmt->texpr_as->ctx;
    nnc_resolve_aliased_type(type_stmt->texpr_as->type, st, ctx);
}

/**
 * @brief Resolves expression statement.
 * @param expr_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when declared outside of function.
 */
nnc_static void nnc_resolve_expr_stmt(nnc_expression_statement* expr_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_FN)) {
        THROW(NNC_SEMANTIC, "cannot use expression in this context.");
    }
    nnc_resolve_expr(expr_stmt->expr, st);
}

/**
 * @brief Resolves while statement.
 * @param while_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when declared outside of function.
 */
nnc_static void nnc_resolve_while_stmt(nnc_while_statement* while_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_FN)) {
        THROW(NNC_SEMANTIC, "cannot declare `while` in this context.");
    }
    nnc_resolve_condition_expr(while_stmt->cond, st);
    nnc_resolve_stmt(while_stmt->body, st);
}

/**
 * @brief Resolves break statement.
 * @param break_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when declared outside of loop.
 */
nnc_static void nnc_resolve_break_stmt(nnc_break_statement* break_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_LOOP)) {
        THROW(NNC_SEMANTIC, "cannot use `break` outside loop.", break_stmt->ctx);
    }
}

/**
 * @brief Resolves return statement.
 * @param ret_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when declared outside of function.
 *        `NNC_SEMANTIC` in case when return type differs with expression type.
 *        `NNC_SEMANTIC` in case when returning nothing from non void function.
 */
nnc_static void nnc_resolve_return_stmt(nnc_return_statement* ret_stmt, nnc_st* st) {
    nnc_st* t_st = NULL;
    if (!nnc_st_has_ctx(st, &t_st, ST_CTX_FN)) {
        THROW(NNC_SEMANTIC, "cannot use `return` outside function.");
    }
    assert(t_st != NULL);
    nnc_resolve_stmt(ret_stmt->body, st);
    const nnc_type* t_fn_ret = t_st->ref.fn->ret->type;
    if (ret_stmt->body->kind == STMT_EXPR) {
        const nnc_expression_statement* expr_stmt = ret_stmt->body->exact;
        const nnc_type* t_ret = nnc_expr_get_type(expr_stmt->expr);
        if (!nnc_can_imp_cast_assign(t_ret, t_fn_ret)) {
            THROW(NNC_SEMANTIC, sformat("cannot return value of type `%s` when function has `%s` return type.",
                nnc_type_tostr(t_ret), nnc_type_tostr(t_fn_ret)), ret_stmt->ctx);    
        }
    }
    else {
        // check if function return type is void or not
        // because we returning nothing here.
        if (t_fn_ret->kind != T_VOID) {
            THROW(NNC_SEMANTIC, "cannot return nothing from non-void function.", ret_stmt->ctx);
        }
    }
}

/**
 * @brief Resolves compound statement.
 * @param compound_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolve_compound_stmt(nnc_compound_statement* compound_stmt, nnc_st* st) {
    nnc_u64 len = buf_len(compound_stmt->stmts);
    for (nnc_u64 i = 0; i < len; i++) {
        TRY {
            nnc_resolve_stmt(compound_stmt->stmts[i], compound_stmt->scope);
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(&CATCHED.where);
        }
    }
}

/**
 * @brief Resolves continue statement.
 * @param continue_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when declared outside loop. 
 */
nnc_static void nnc_resolve_continue_stmt(nnc_continue_statement* continue_stmt, nnc_st* st) {
    if (!nnc_st_has_ctx(st, NULL, ST_CTX_LOOP)) {
        THROW(NNC_SEMANTIC, "cannot use `continue` outside loop.", continue_stmt->ctx);
    }
}

/**
 * @brief Resolves namespace statement.
 * @param namespace_stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 * @throw `NNC_SEMANTIC` in case when not declared in global scope. 
 */
nnc_static void nnc_resolve_namespace_stmt(nnc_namespace_statement* namespace_stmt, nnc_st* st) {
    if (st->ctx != ST_CTX_GLOBAL &&
        st->ctx != ST_CTX_NAMESPACE) {
        THROW(NNC_SEMANTIC, "cannot declare `namespace` in this context.", namespace_stmt->var->ctx);
    }
    nnc_resolve_stmt(namespace_stmt->body, st);
}

/**
 * @brief Generates three-address code from resolved statement.
 *  Statement must be STMT_FN or STMT_LET in global or namespace scope.
 * @param stmt Statement to be converted to three-address code.
 * @param st Pointer to `nnc_st` instance.
 */
nnc_static void nnc_resolved_stmt_to_3a(const nnc_statement* stmt, const nnc_st* st) {
    nnc_heap_ptr exact = stmt->exact;
    if (st->ctx != ST_CTX_GLOBAL) {
        return;
    }
    if (stmt->kind == STMT_FN) {
        nnc_fn_storage storage = ((nnc_fn_statement*)exact)->storage;
        if (storage != FN_ST_EXTERN) {
            nnc_stmt_to_3a(stmt, st);
        }
    }
    if (stmt->kind == STMT_LET ||
        stmt->kind == STMT_NAMESPACE) {
        nnc_stmt_to_3a(stmt, st);
    }
}

/**
 * @brief Resolves statement.
 * @param stmt Statement to be resolved.
 * @param st Pointer to `nnc_st` instance.
 */
void nnc_resolve_stmt(nnc_statement* stmt, nnc_st* st) {
    switch (stmt->kind) {
        case STMT_EMPTY:     break;
        case STMT_DO:        nnc_resolve_do_stmt(stmt->exact, st);        break;
        case STMT_FN:        nnc_resolve_fn_stmt(stmt->exact, st);        break;
        case STMT_IF:        nnc_resolve_if_stmt(stmt->exact, st);        break;
        case STMT_FOR:       nnc_resolve_for_stmt(stmt->exact, st);       break;
        case STMT_LET:       nnc_resolve_let_stmt(stmt->exact, st);       break;
        case STMT_TYPE:      nnc_resolve_type_stmt(stmt->exact, st);      break;
        case STMT_EXPR:      nnc_resolve_expr_stmt(stmt->exact, st);      break;
        case STMT_WHILE:     nnc_resolve_while_stmt(stmt->exact, st);     break;
        case STMT_BREAK:     nnc_resolve_break_stmt(stmt->exact, st);     break;
        case STMT_RETURN:    nnc_resolve_return_stmt(stmt->exact, st);    break;
        case STMT_EXT_FN:    nnc_resolve_fn_stmt(stmt->exact, st);        break;
        case STMT_COMPOUND:  nnc_resolve_compound_stmt(stmt->exact, st);  break;
        case STMT_CONTINUE:  nnc_resolve_continue_stmt(stmt->exact, st);  break;
        case STMT_NAMESPACE: nnc_resolve_namespace_stmt(stmt->exact, st); break;
        default: {
            nnc_abort_no_ctx("nnc_resolve_stmt: unknown kind.\n");
        }
    }
    //if (!nnc_error_occured()) {
    //    nnc_resolved_stmt_to_3a(stmt, st);
    //}
}

/**
 * @brief Resolves abstract syntax tree of the program (semantic analysis).
 * @param ast AST to be resolved.
 */
void nnc_resolve(nnc_ast* ast) {
    code = NULL;
    for (nnc_u64 i = 0; i < buf_len(ast->root); i++) {
        TRY {
            nnc_resolve_stmt(ast->root[i], ast->st);
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(&CATCHED.where);
        }
    }
}