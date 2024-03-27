#ifndef __NNC_AST_LITERALS_H__
#define __NNC_AST_LITERALS_H__

#include "nnc_type.h"
#include "nnc_try_catch.h"

#define dbl_literal(expr) (nnc_dbl_literal*)expr->exact
#define int_literal(expr) (nnc_int_literal*)expr->exact
#define chr_literal(expr) (nnc_chr_literal*)expr->exact
#define str_literal(expr) (nnc_str_literal*)expr->exact

#define SUFFIX_NONE 0xf

typedef enum _nnc_dbl_suffix {
    SUFFIX_F32, SUFFIX_F64
} nnc_dbl_suffix;

typedef struct _nnc_dbl_literal {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_f64 exact;
    nnc_dbl_suffix suffix;
} nnc_dbl_literal;

typedef enum _nnc_int_suffix {
    SUFFIX_U8, SUFFIX_U16, SUFFIX_U32, SUFFIX_U64,
    SUFFIX_I8, SUFFIX_I16, SUFFIX_I32, SUFFIX_I64
} nnc_int_suffix;

typedef struct _nnc_int_literal {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_byte base;
    nnc_bool is_signed;
    nnc_int_suffix suffix;
    union {
        nnc_u64 u;
        nnc_i64 d;
    } exact;
} nnc_int_literal;

typedef struct _nnc_chr_literal {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_byte exact;
} nnc_chr_literal;

typedef struct _nnc_str_literal {
    nnc_ctx ctx;
    nnc_type* type;
    nnc_u64 bytes;
    nnc_byte* exact;
} nnc_str_literal;

typedef struct _nnc_bounds {
    union {
        nnc_i64 d;
        nnc_u64 u;
        nnc_f64 f;
    } min;
    union {
        nnc_i64 d;
        nnc_u64 u;
        nnc_f64 f;
    } max;
} nnc_bounds;

/**
 * @brief Allocates & initializes new instance of `nnc_dbl_literal`.
 * @param repr String representation of the float number which will be parsed.
 * @param ctx Context of the double literal.
 * @return Allocated & initialized instance of `nnc_dbl_literal`.
 */
nnc_dbl_literal* nnc_dbl_new(
    const char* repr,
    const nnc_ctx* ctx
);

/**
 * @brief Allocates & initializes new instance of `nnc_int_literal`.
 * @param repr String representation of the float number which will be parsed.
 * @param ctx Context of int literal.
 * @return Allocated & initialized instance of `nnc_int_literal`.
 * @throw NNC_OVERFLOW in case of `errno == ENOENT` after call to `strtoll` or `strtoull`.
 */
nnc_int_literal* nnc_int_new(
    const char* repr,
    const nnc_ctx* ctx
);

/**
 * @brief Allocates & initializes new instance of `nnc_chr_literal`.
 * @param repr String representation of char literal which will be parsed.
 * @param ctx Context of the character literal.
 * @return Allocated & initialized instance of `nnc_chr_literal`.
 */
nnc_chr_literal* nnc_chr_new(
    const char* repr,
    const nnc_ctx* ctx
);

/**
 * @brief Allocates & initializes new instance of `nnc_str_literal`.
 * @param repr String representation of string literal which will be parsed.
 * @param ctx Context of the string literal.
 * @return Allocated & initialized instance of `nnc_str_literal`.
 */
nnc_str_literal* nnc_str_new(
    const char* repr,
    const nnc_ctx* ctx
);

/**
 * @brief Disposes float literal instance.
 * @param literal Float literal instance to be disposed.
 */
void nnc_dbl_free(
    nnc_dbl_literal* literal
);

/**
 * @brief Disposes integral literal instance.
 * @param literal Integral literal instance to be disposed.
 */
void nnc_int_free(
    nnc_int_literal* literal
);

/**
 * @brief Disposes character literal instance.
 * @param literal Character literal instance to be disposed.
 */
void nnc_chr_free(
    nnc_chr_literal* literal
);

/**
 * @brief Disposes string literal instance.
 * @param literal String literal instance to be disposed.
 */
void nnc_str_free(
    nnc_str_literal* literal
);

#endif