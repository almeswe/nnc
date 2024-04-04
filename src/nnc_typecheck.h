#ifndef __NNC_TYPECHECK_H__
#define __NNC_TYPECHECK_H__

#include "nnc_ast.h"

/**
 * @brief Resolves & determines size of specified type. (in bytes)
 * @param type Pointer to `nnc_type` instance.
 * @return Size of type. 
 */
nnc_u64 nnc_sizeof(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is function.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is function, otherwise `false`.
 */
nnc_bool nnc_fn_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is pointer type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is pointer, otherwise `false`.
 */
nnc_bool nnc_ptr_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is array type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is array, otherwise `false`.
 */
nnc_bool nnc_arr_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is namespace type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is namespace, otherwise `false`.
 */
nnc_bool nnc_numeric_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is real.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is real, otherwise `false`.
 */
nnc_bool nnc_real_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is signed.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is signed, otherwise `false`.
 */
nnc_bool nnc_signed_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is unsigned.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is unsigned, otherwise `false`.
 */
#define nnc_unsigned_type(...) !nnc_signed_type(__VA_ARGS__)

/**
 * @brief Checks if specified type is integral.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is integral, otherwise `false`.
 */
nnc_bool nnc_integral_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is namespace type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is namespace, otherwise `false`.
 */
nnc_bool nnc_namespace_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is array or pointer type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is array or pointer, otherwise `false`.
 */
nnc_bool nnc_arr_or_ptr_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is incomplete.
 *  Performs broad check for `T_VOID`, `T_UNKNOWN` & `T_INCOMPLETE`.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is imcomplete, otherwise `false`.
 */
nnc_bool nnc_incomplete_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is struct or union type.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is struct or union, otherwise `false`.
 */
nnc_bool nnc_struct_or_union_type(
    const nnc_type* type
);

/**
 * @brief Checks if specified type is primitive.
 *  This function is not sensitive for alias types.
 * @param type Pointer to `nnc_type` to be checked.
 * @return `true` if type is primitive, otherwise `false`.
 */
nnc_bool nnc_primitive_type(
    const nnc_type* type
);

/**
 * @brief Performs type unaliasing. May be not complete unaliasing,
 *  but at least it guarantees that returned type will be no `T_ALIAS`.
 *  After, returned type can be unaliased again, to get pure base type.
 * @param type Pointer to `nnc_type` to be unaliased.
 * @return Pointer to type that is not `T_ALIAS`.
 */
nnc_type* nnc_unalias(
    const nnc_type* type
);

/**
 * @brief Infers type for generic expression.
 * @param expr Pointer to `nnc_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_type* nnc_expr_infer_type(
    nnc_expression* expr,
    nnc_st* st
);

/**
 * @brief Infers type for binary expression.
 * @param expr Pointer to `nnc_binary_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_type* nnc_binary_expr_infer_type(
    nnc_binary_expression* expr,
    nnc_st* st
);

/**
 * @brief Infers type for ternary expression.
 * @param expr Pointer to `nnc_ternary_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`.
 */
nnc_type* nnc_ternary_expr_infer_type(
    nnc_ternary_expression* expr,
    nnc_st* st
);

/**
 * @brief Gets type from generic expression.
 * @param expr Pointer to `nnc_expression`. 
 * @param st Pointer to `nnc_st`.
 * @return Pointer to resulting `nnc_type`, or `unknown_type` if something went wrong.
 */
nnc_type* nnc_expr_get_type(
    const nnc_expression* expr
);

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
nnc_bool nnc_can_imp_cast_assign(
    const nnc_type* from,
    const nnc_type* to
);

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
nnc_bool nnc_can_imp_cast_arith(
    const nnc_type* from,
    const nnc_type* to
);

/**
 * @brief Checks if explicit cast of two types is possible.
 *  This function is not sensitive for alias types.
 * @param from Pointer to `nnc_type` from which to cast.
 * @param to Pointer to `nnc_type` to cast to.
 * @return `true` if types can be casted explicitly, otherwise `false`.
 */
nnc_bool nnc_can_exp_cast(
    const nnc_type* from,
    const nnc_type* to
);

#endif