#include "nnc_typecheck.h"

/**
 * @brief Checks if type has at least one specifier 
 *  (of type `nnc_type_kind`) listed in var args.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type fits at least one specifier, otherwise `false`. 
 */
#define T_IS(t, ...) nnc_type_is(t, __VA_ARGS__, -1)

/**
 * @brief Performs type unaliasing, and creating of local variable
 *  of type `nnc_type` and named `ref_`+`type`. 
 * @param type Pointer to `nnc_type` to be unaliased.
 */
#define T_UNALIAS(type) nnc_type* ref_##type = nnc_unalias(type)

/**
 * @brief Checks if type has at least one specifier 
 *  (of type `nnc_type_kind`) listed in var args.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type fits at least one specifier, otherwise `false`. 
 */
nnc_static nnc_bool nnc_type_is(const nnc_type* type, ...) {
    va_list list = {0};
    va_start(list, type);
    for (;;) {
        nnc_i32 arg = va_arg(list, nnc_i32);
        if (arg == -1) {
            break;
        }
        nnc_type_kind kind = arg;
        if (kind == type->kind) {
            va_end(list);
            return true;
        }
    }
    va_end(list);
    return false;
}

/**
 * @brief Performs type unaliasing. May be not complete unaliasing,
 *  but at least it guarantees that returned type will be no `T_ALIAS`.
 *  After, returned type can be unaliased again, to get pure base type.
 * @param type Pointer to `nnc_type` to be unaliased.
 * @return Pointer to type that is not `T_ALIAS`.
 */
nnc_type* nnc_unalias(const nnc_type* type) {
    nnc_type* base = (nnc_type*)type;
    while (base->kind == T_ALIAS) {
        base = base->base;
    }
    return base;
}

/**
 * @brief Checks if specified type is incomplete.
 *  Performs broad check for `T_VOID`, `T_UNKNOWN` & `T_INCOMPLETE`.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is imcomplete, otherwise `false`.
 */
nnc_bool nnc_incomplete_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_VOID    || 
           ref_type->kind == T_UNKNOWN || 
           ref_type->kind == T_INCOMPLETE;
}

/**
 * @brief Checks if specified type is function.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is function, otherwise `false`.
 */
nnc_bool nnc_fn_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_FUNCTION;
}

/**
 * @brief Checks if specified type is pointer type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is pointer, otherwise `false`.
 */
nnc_bool nnc_ptr_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_POINTER;
}

/**
 * @brief Checks if specified type is array type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is array, otherwise `false`.
 */
nnc_bool nnc_arr_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_ARRAY;
}

/**
 * @brief Checks if specified type is namespace type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is namespace, otherwise `false`.
 */
nnc_bool nnc_namespace_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_NAMESPACE;
}

/**
 * @brief Checks if specified type is array or pointer type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is array or pointer, otherwise `false`.
 */
nnc_bool nnc_arr_or_ptr_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_POINTER || 
           ref_type->kind == T_ARRAY;
}

/**
 * @brief Checks if specified type is struct or union type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is struct or union, otherwise `false`.
 */
nnc_bool nnc_struct_or_union_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_STRUCT || 
           ref_type->kind == T_UNION;
}

/**
 * @brief Checks if specified type is primitive.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is primitive, otherwise `false`.
 */
nnc_bool nnc_primitive_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_PRIMITIVE_I8  || 
           ref_type->kind == T_PRIMITIVE_U8  ||
           ref_type->kind == T_PRIMITIVE_I16 || 
           ref_type->kind == T_PRIMITIVE_U16 ||
           ref_type->kind == T_PRIMITIVE_I32 || 
           ref_type->kind == T_PRIMITIVE_U32 ||
           ref_type->kind == T_PRIMITIVE_I64 || 
           ref_type->kind == T_PRIMITIVE_U64 ||
           ref_type->kind == T_PRIMITIVE_F32 || 
           ref_type->kind == T_PRIMITIVE_F64;
}

/**
 * @brief Checks if unaliased instances of two types are equal (by address).
 * @param t1 Pointer to first `nnc_type` to be compared.
 * @param t2 Pointer to second `nnc_type` to be compared.
 * @return `true` if both unaliased types are equal by address, otherwise `false`.
 */
nnc_bool nnc_same_types(const nnc_type* t1, const nnc_type* t2) {
    return nnc_unalias(t1) == nnc_unalias(t2);
}

/**
 * @brief Checks if specified type is integral.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is integral, otherwise `false`.
 */
nnc_bool nnc_integral_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_PRIMITIVE_I8  || 
           ref_type->kind == T_PRIMITIVE_U8  ||
           ref_type->kind == T_PRIMITIVE_I16 || 
           ref_type->kind == T_PRIMITIVE_U16 ||
           ref_type->kind == T_PRIMITIVE_I32 || 
           ref_type->kind == T_PRIMITIVE_U32 ||
           ref_type->kind == T_PRIMITIVE_I64 || 
           ref_type->kind == T_PRIMITIVE_U64;
}

/**
 * @brief Checks if specified type is real.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is real, otherwise `false`.
 */
nnc_bool nnc_real_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_PRIMITIVE_F32  || 
           ref_type->kind == T_PRIMITIVE_F64;
}

/**
 * @brief Checks if specified type is signed.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is signed, otherwise `false`.
 */
nnc_bool nnc_signed_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return ref_type->kind == T_ENUM          ||
           ref_type->kind == T_PRIMITIVE_I8  || 
           ref_type->kind == T_PRIMITIVE_I16 || 
           ref_type->kind == T_PRIMITIVE_I32 || 
           ref_type->kind == T_PRIMITIVE_I64;
}

/**
 * @brief Checks if specified type is numeric (float, integral or enum).
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is numeric, otherwise `false`.
 */
nnc_bool nnc_numeric_type(const nnc_type* type) {
    const T_UNALIAS(type);
    return nnc_integral_type(ref_type)    ||
        nnc_real_type(ref_type)           ||
        ref_type->kind == T_ENUM;
}

/**
 * @brief Gets rank of array or pointer type.
 * @param type Pointer to `nnc_type` to be checked.
 * @return Rank of array or pointer type.
 */
const nnc_i32 nnc_rank(const nnc_type* type) {
    nnc_i32 rank = 0;
    const T_UNALIAS(type);
    while (nnc_arr_or_ptr_type(ref_type)) {
        rank++;
        ref_type = ref_type->base;
    }
    return rank;
}

/**
 * @brief Checks if one type can be casted to another type arithmetically.
 *  This means, types are firstly checked if they both numeric or aggregate (struct or union).
 *  If both are numeric, priorities of both are checked.
 *  If both are struct or union, they are checked by actual address.
 *   This keeps equality check of user-defined types simple, because
 *   in most cases struct or union types are aliased and both these alias types 
 *   point to same struct or union type.
 *  This function is not sensitive for alias types.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return `true` if types can be casted implicitly, otherwise `false`.
 */
nnc_bool nnc_can_imp_cast_arith(const nnc_type* from, const nnc_type* to) {
    // todo: maybe rename this function (due to fact that 
    // this is not pure arithmetic (structs and unions involved))
    T_UNALIAS(to); T_UNALIAS(from);
    static const nnc_i32 hierarchy[] = {
        [T_PRIMITIVE_I8]  = 0,
        [T_PRIMITIVE_U8]  = 1,
        [T_PRIMITIVE_I16] = 2,
        [T_PRIMITIVE_U16] = 3,
        [T_PRIMITIVE_I32] = 4,
        [T_PRIMITIVE_U32] = 5,
        [T_PRIMITIVE_I64] = 6,
        [T_ENUM]          = 6,
        [T_PRIMITIVE_U64] = 7,
        [T_PRIMITIVE_F32] = 8,
        [T_PRIMITIVE_F64] = 9
    };
    if (!nnc_numeric_type(ref_to) || 
        !nnc_numeric_type(ref_from)) {
        if (!nnc_struct_or_union_type(ref_to) ||
            !nnc_struct_or_union_type(ref_from)) {
            return false;
        }
        return nnc_same_types(ref_to, ref_from);
    }
    return hierarchy[ref_from->kind] <= hierarchy[ref_to->kind];
}

/**
 * @brief Performs implicit arithmetic cast.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return Pointer to type to cast to, if `nnc_can_imp_cast_arith` returned `true`.
 * @throw `NNC_SEMANTIC` in case when types cannot be casted impilicitly.
 */
nnc_type* nnc_cast_imp_arith(const nnc_type* from, const nnc_type* to) {
    if (!nnc_can_imp_cast_arith(from, to)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast `%s` to `%s`.", 
            nnc_type_tostr(from), nnc_type_tostr(to)));
    }
    return (nnc_type*)to;
}

/**
 * @brief Checks if one type can be casted to another type if they are both pointers.
 *  Types checked by rank and they base types are passed to `nnc_can_imp_cast_arith`.
 *  If at least one base type is `T_VOID`, `true` is returned.
 *  This function is sensitive for alias types.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return `true` if types can be casted implicitly, otherwise `false`.
 */
nnc_bool nnc_can_imp_cast_ptrs(const nnc_type* from, const nnc_type* to) {
    if (nnc_rank(from) != nnc_rank(to)) {
        return false;
    }
    const nnc_type* ref_to = to;
    const nnc_type* ref_from = from;
    while (T_IS(ref_to, T_ARRAY, T_ALIAS, T_POINTER)) {
        ref_to = ref_to->base;
    }
    while (T_IS(ref_from, T_ARRAY, T_ALIAS, T_POINTER)) {
        ref_from = ref_from->base;
    }
    if (ref_to->kind == T_VOID || 
        ref_from->kind == T_VOID) {
        return true;
    }
    return nnc_can_imp_cast_arith(ref_from, ref_to);
}

/**
 * @brief Checks if one type can be casted to another type if they are both functions.
 *  All parameter and return types of both function types
 *  are passed to `nnc_can_imp_cast_arith`.
 *  This function is not sensitive for alias types.
 * @param t1 Pointer to first `nnc_type`.
 * @param t2 Pointer to second `nnc_type`.
 * @return `true` if types can be casted implicitly, otherwise `false`.
 */
nnc_bool nnc_can_imp_cast_fns(const nnc_type* t1, const nnc_type* t2) {
    T_UNALIAS(t1); T_UNALIAS(t2);
    assert(ref_t1->kind == T_FUNCTION);
    assert(ref_t2->kind == T_FUNCTION);
    if (ref_t1->exact.fn.paramc != ref_t2->exact.fn.paramc) {
        return false;
    }
    nnc_type_expression** t1_params = ref_t1->exact.fn.params;
    nnc_type_expression** t2_params = ref_t2->exact.fn.params;
    for (nnc_u64 i = 0; i < ref_t1->exact.fn.paramc; i++) {
        const nnc_type* t1_param = t1_params[i]->type;
        const nnc_type* t2_param = t2_params[i]->type;
        if (!nnc_can_imp_cast_assign(t2_param, t1_param)) {
            return false;
        }
    }
    const nnc_type* t1_ret = ref_t1->exact.fn.ret->type;
    const nnc_type* t2_ret = ref_t2->exact.fn.ret->type;
    return nnc_can_imp_cast_assign(t2_ret, t1_ret);
}

/**
 * @brief Checks if one type can be casted to another type if they are in assignment expression.
 *  There rules are used:
 *   1) `numeric` = `numeric`
 *   2) `pointer` = `pointer`
 *   3) `pointer` = `array`
 *   4) `function` = `function`
 *   5) `struct-or-union` = `struct-or-union`
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return `true` if types can be casted implicitly, otherwise `false`.
 */
nnc_bool nnc_can_imp_cast_assign(const nnc_type* from, const nnc_type* to) {
    T_UNALIAS(to); T_UNALIAS(from);
    if (nnc_can_imp_cast_arith(ref_from, ref_to)) {
        return nnc_cast_imp_arith(ref_from, ref_to);
    }
    if (nnc_fn_type(ref_to) && nnc_fn_type(ref_from)) {
        return nnc_can_imp_cast_fns(ref_to, ref_from);
    }
    if (nnc_ptr_type(ref_to) && nnc_arr_or_ptr_type(ref_from)) {
        return nnc_can_imp_cast_ptrs(ref_from, ref_to);
    }
    return false;
}

/**
 * @brief Performs implicit cast of two types.
 *  This function is not sensitive to the order in which you pass both types.
 * @param t1 Pointer to first `nnc_type`.
 * @param t2 Pointer to second `nnc_type`.
 * @return Pointer to `nnc_type` which represents casted type.
 * @throw `NNC_SEMANTIC` in case when none of the attempts to cast implicitly succeeded. 
 */
nnc_type* nnc_infer_imp(const nnc_type* t1, const nnc_type* t2) {
    if (nnc_can_imp_cast_arith(t1, t2)) {
        return nnc_cast_imp_arith(t1, t2);
    }
    if (nnc_can_imp_cast_arith(t2, t1)) {
        return nnc_cast_imp_arith(t2, t1);
    }
    THROW(NNC_SEMANTIC, sformat("implicit cast between `%s` and `%s` is not possible.", 
        nnc_type_tostr(t1), nnc_type_tostr(t2)));
    return NULL;
}

/**
 * @brief Performs implicit cast of two types located in assignment expression.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return Pointer to `nnc_type` which represents casted type.
 * @throw `NNC_SEMANTIC` in case when `nnc_can_imp_cast_assign` returned `false`.
 */
nnc_type* nnc_infer_imp_assign(const nnc_type* from, const nnc_type* to) {
    if (!nnc_can_imp_cast_assign(from, to)) {
        THROW(NNC_SEMANTIC, sformat("cannot cast `%s` to `%s`.", 
            nnc_type_tostr(from), nnc_type_tostr(to)));
    }
    return (nnc_type*)to;
}

/**
 * @brief Checks if explicit cast of two types is possible.
 *  This function is not sensitive for alias types.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return `true` if types can be casted explicitly, otherwise `false`.
 */
nnc_bool nnc_can_exp_cast(const nnc_type* from, const nnc_type* to) {
    T_UNALIAS(to); T_UNALIAS(from);
    if (ref_to->kind == T_ARRAY ||
        ref_to->kind == T_VOID) {
        return false;
    }
    if (nnc_struct_or_union_type(ref_to) ||
        nnc_struct_or_union_type(ref_from)) {
        return nnc_same_types(ref_to, ref_from);
    }
    return true;
}

/**
 * @brief Performs explicit cast of two types.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return Pointer to `nnc_type` which represents casted type.
 * @throw `NNC_SEMANTIC` in case when `nnc_can_exp_cast` returned `false`.
 */
nnc_type* nnc_exp_cast(const nnc_type* from, const nnc_type* to) {
    if (nnc_can_exp_cast(from, to)) {
        return (nnc_type*)to;
    }
    THROW(NNC_SEMANTIC, sformat("cannot cast `%s` to `%s`.", 
        nnc_type_tostr(from), nnc_type_tostr(to)));
    return NULL;
} 

/**
 * @brief Infers type for character literal.
 * @param literal Pointer to `nnc_chr_literal`.
 * @return Pointer to `nnc_type` which is type of literal.
 */
nnc_static nnc_type* nnc_chr_infer_type(nnc_chr_literal* literal) {
    return literal->type = &u8_type;
}

/**
 * @brief Infers type for string literal.
 * @param literal Pointer to `nnc_str_literal`.
 * @return Pointer to `nnc_type` which is type of literal.
 */
nnc_static nnc_type* nnc_str_infer_type(nnc_str_literal* literal) {
    return literal->type = nnc_ptr_type_new(&u8_type);
}

/**
 * @brief Infers type for integral literal.
 * @param literal Pointer to `nnc_int_literal`.
 * @return Pointer to `nnc_type` which is type of literal.
 */
nnc_static nnc_type* nnc_int_infer_type(nnc_int_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_I8:  return literal->type = &i8_type;
        case SUFFIX_U8:  return literal->type = &u8_type;
        case SUFFIX_I16: return literal->type = &i16_type;
        case SUFFIX_U16: return literal->type = &u16_type;
        case SUFFIX_I32: return literal->type = &i32_type;
        case SUFFIX_U32: return literal->type = &u32_type;
        case SUFFIX_I64: return literal->type = &i64_type;
        case SUFFIX_U64: return literal->type = &u64_type;
        default: {
            nnc_abort("nnc_int_infer_type: unknown suffix.\n", &literal->ctx);
        }
    }
    return &unknown_type;
}

/**
 * @brief Infers type for floating-pointliteral.
 * @param literal Pointer to `nnc_dbl_literal`.
 * @return Pointer to `nnc_type` which is type of literal.
 */
nnc_static nnc_type* nnc_dbl_infer_type(nnc_dbl_literal* literal) {
    switch (literal->suffix) {
        case SUFFIX_F32: return literal->type = &f32_type;
        case SUFFIX_F64: return literal->type = &f64_type;
        default: {
            nnc_abort("nnc_dbl_infer_type: unknown suffix.\n", &literal->ctx);
        }
    }
    return &unknown_type;
}

/**
 * @brief Infers type for identifier.
 * @param ident Pointer to `nnc_ident`.
 * @return Pointer to `nnc_type` which is type of identifer.
 */
nnc_static nnc_type* nnc_ident_infer_type(nnc_ident* ident) {
    //todo: i'm not sure about this yet
    //if (ident->ictx == IDENT_NAMESPACE) {
    //    return &unknown_type;
    //}
    return ident->type;
}

/**
 * @brief Infers type for unary expression.
 * @param expr Pointer to `nnc_unary_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_static nnc_type* nnc_unary_expr_infer_type(nnc_unary_expression* expr, nnc_st* st) {
    const nnc_type* t_expr = nnc_expr_infer_type(expr->expr, st);
    if (expr->kind == UNARY_CAST ||
        expr->kind == UNARY_POSTFIX_AS) {
        return expr->type = nnc_exp_cast(t_expr, expr->exact.cast.to->type);
    }
    return &unknown_type;
}

/**
 * @brief Infers type for binary expression.
 * @param expr Pointer to `nnc_binary_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_type* nnc_binary_expr_infer_type(nnc_binary_expression* expr, nnc_st* st) {
    nnc_type* t_lexpr = nnc_expr_infer_type(expr->lexpr, st);
    nnc_type* t_rexpr = nnc_expr_infer_type(expr->rexpr, st);
    switch (expr->kind) {
        case BINARY_COMMA:
            return expr->type = t_rexpr;
        case BINARY_ASSIGN:
            return expr->type = nnc_infer_imp_assign(t_rexpr, t_lexpr);
        case BINARY_MOD:
        case BINARY_ADD: case BINARY_SUB:
        case BINARY_MUL: case BINARY_DIV:
        case BINARY_SHL: case BINARY_SHR:
            return expr->type = nnc_infer_imp(t_lexpr, t_rexpr);
        case BINARY_BW_OR:
        case BINARY_BW_AND: case BINARY_BW_XOR:
            return expr->type = nnc_infer_imp(t_lexpr, t_rexpr);
        case BINARY_OR:  case BINARY_AND:
        case BINARY_EQ:  case BINARY_NEQ:
        case BINARY_GT:  case BINARY_LT:
        case BINARY_GTE: case BINARY_LTE:
            // return value is not used here
            // just need to be sure that types are the same
            expr->type = nnc_infer_imp(t_lexpr, t_rexpr);
            return nnc_unsigned_type(expr->type) ? &u8_type : &i8_type;
        default:
            return &unknown_type;
    }
}

/**
 * @brief Infers type for ternary expression.
 * @param expr Pointer to `nnc_ternary_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`.
 */
nnc_type* nnc_ternary_expr_infer_type(nnc_ternary_expression* expr, nnc_st* st) {
    nnc_type* t_lexpr = nnc_expr_infer_type(expr->lexpr, st);
    nnc_type* t_rexpr = nnc_expr_infer_type(expr->rexpr, st);
    if (nnc_can_imp_cast_assign(t_rexpr, t_lexpr)) {
        return expr->type = nnc_infer_imp_assign(t_rexpr, t_lexpr);
    }
    return expr->type = nnc_infer_imp_assign(t_lexpr, t_rexpr);
}

/**
 * @brief Infers type for generic expression.
 * @param expr Pointer to `nnc_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_type* nnc_expr_infer_type(nnc_expression* expr, nnc_st* st) {
    nnc_type* t_expr = nnc_expr_get_type(expr);
    if (t_expr->kind != T_UNKNOWN) {
        return t_expr;
    }
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: return nnc_chr_infer_type(expr->exact);
        case EXPR_STR_LITERAL: return nnc_str_infer_type(expr->exact);
        case EXPR_DBL_LITERAL: return nnc_dbl_infer_type(expr->exact);
        case EXPR_INT_LITERAL: return nnc_int_infer_type(expr->exact);
        case EXPR_IDENT:       return nnc_ident_infer_type(expr->exact);
        case EXPR_UNARY:       return nnc_unary_expr_infer_type(expr->exact, st);
        case EXPR_BINARY:      return nnc_binary_expr_infer_type(expr->exact, st);
        default: {
            nnc_abort("nnc_expr_infer_type: unknown kind.\n", nnc_expr_get_ctx(expr));
        }
    }
    return &unknown_type;
}

/**
 * @brief Gets type from generic expression.
 * @param expr Pointer to `nnc_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_type* nnc_expr_get_type(const nnc_expression* expr) {
    nnc_type* t_expr = NULL;
    switch (expr->kind) {
        case EXPR_CHR_LITERAL: t_expr = ((nnc_chr_literal*)expr->exact)->type;        break;
        case EXPR_STR_LITERAL: t_expr = ((nnc_str_literal*)expr->exact)->type;        break;
        case EXPR_DBL_LITERAL: t_expr = ((nnc_dbl_literal*)expr->exact)->type;        break;
        case EXPR_INT_LITERAL: t_expr = ((nnc_int_literal*)expr->exact)->type;        break;
        case EXPR_IDENT:       t_expr = ((nnc_ident*)expr->exact)->type;              break;
        case EXPR_UNARY:       t_expr = ((nnc_unary_expression*)expr->exact)->type;   break;
        case EXPR_BINARY:      t_expr = ((nnc_binary_expression*)expr->exact)->type;  break;
        case EXPR_TERNARY:     t_expr = ((nnc_ternary_expression*)expr->exact)->type; break;
        default: {
            nnc_abort("nnc_expr_get_type: unknown kind.\n", nnc_expr_get_ctx(expr));
        }
    }
    return t_expr == NULL ? &unknown_type : nnc_unalias(t_expr); 
}