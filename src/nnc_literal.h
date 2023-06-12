#ifndef _NNC_AST_LITERALS_H
#define _NNC_AST_LITERALS_H

#include "nnc_types.h"
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
    nnc_f64 exact;
    nnc_dbl_suffix suffix;
} nnc_dbl_literal;

typedef enum _nnc_int_suffix {
    SUFFIX_U8, SUFFIX_U16, SUFFIX_U32, SUFFIX_U64,
    SUFFIX_I8, SUFFIX_I16, SUFFIX_I32, SUFFIX_I64
} nnc_int_suffix;

typedef struct _nnc_int_literal {
    nnc_byte base;
    nnc_bool is_signed;
    nnc_int_suffix suffix;
    union {
        nnc_u64 u;
        nnc_i64 d;
    } exact;
} nnc_int_literal;

typedef struct _nnc_chr_literal {
    nnc_byte exact;
} nnc_chr_literal;

typedef struct _nnc_str_literal {
    nnc_u64 length;
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

nnc_dbl_literal* nnc_dbl_new(const char* repr);
nnc_int_literal* nnc_int_new(const char* repr);
nnc_chr_literal* nnc_chr_new(const char* repr);
nnc_str_literal* nnc_str_new(const char* repr);

void nnc_dbl_free(nnc_dbl_literal* literal);
void nnc_int_free(nnc_int_literal* literal);
void nnc_chr_free(nnc_chr_literal* literal);
void nnc_str_free(nnc_str_literal* literal);

#endif