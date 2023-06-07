#ifndef _NNC_AST_LITERALS_H
#define _NNC_AST_LITERALS_H

#include "nnc_types.h"

#define dbl_literal(expr) (nnc_dbl_literal*)expr->exact
#define int_literal(expr) (nnc_int_literal*)expr->exact
#define chr_literal(expr) (nnc_chr_literal*)expr->exact
#define str_literal(expr) (nnc_str_literal*)expr->exact

typedef enum _nnc_dbl_endian {
    ENDIAN_F32, ENDIAN_F64
} nnc_dbl_endian;

typedef struct _nnc_dbl_literal {
    nnc_f64 exact;
    nnc_dbl_endian endian;
} nnc_dbl_literal;

typedef enum _nnc_int_endian {
    ENDIAN_U8, ENDIAN_U16, ENDIAN_U32, ENDIAN_U64,
    ENDIAN_I8, ENDIAN_I16, ENDIAN_I32, ENDIAN_I64
} nnc_int_endian;

typedef struct _nnc_int_literal {
    nnc_byte base;
    nnc_bool has_sign;
    nnc_int_endian endian;
    union {
        nnc_u64 u;
        nnc_i64 d;
    } exact;
} nnc_int_literal;

typedef struct _nnc_chr_literal {
    nnc_byte code;
    nnc_bool has_escape_seq;
} nnc_chr_literal;

typedef struct _nnc_str_literal {
    char* exact;
    nnc_u64 size;
} nnc_str_literal;

nnc_dbl_literal* nnc_dbl_new(const char* repr);
nnc_int_literal* nnc_int_new(const char* repr);
nnc_chr_literal* nnc_chr_new(const char* repr);
nnc_str_literal* nnc_str_new(const char* repr);

#endif