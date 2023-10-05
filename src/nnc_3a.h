#ifndef _NNC_THREE_ADDRESS_CODE_H
#define _NNC_THREE_ADDRESS_CODE_H

#include "nnc_symtable.h"

typedef nnc_u64 nnc_3a_cgt_cnt;

typedef enum _nnc_3a_op_kind {
    OP_ADD,    OP_SUB,    OP_MUL,
    OP_DIV,    OP_MOD,    OP_SHR,
    OP_SHL,    OP_LTE,    OP_GTE,
    OP_LT,     OP_GT,     OP_EQ,
    OP_OR,     OP_NEQ,    OP_AND,
    OP_BW_AND, OP_BW_XOR, OP_BW_OR,
    OP_MINUS,
    OP_COPY,
    OP_JUMP,
    OP_CJUMP,
    OP_CJUMP2,
    OP_ARG,
    OP_CALL,
    OP_INDEX,
    OP_REF,
    OP_DEREF
} nnc_3a_op_kind;

typedef enum _nnc_3a_addr_kind {
    ADDR_NONE,
    ADDR_NAME,
    ADDR_ICONST,
    ADDR_FCONST,
    ADDR_CGT
} nnc_3a_addr_kind;

typedef nnc_u64         nnc_3a_target; 
typedef nnc_sym         nnc_3a_name;
typedef nnc_int_literal nnc_3a_iconst;
typedef nnc_dbl_literal nnc_3a_fconst;
typedef nnc_u64         nnc_3a_cgt;

#define nnc_3a_cgt_new()       (nnc_3a_addr) { .kind = ADDR_CGT, .exact.cgt = ++cgt_cnt }
#define nnc_3a_name_new(sym)   (nnc_3a_addr) { .kind = ADDR_NAME, .exact.name = *sym } 
#define nnc_3a_iconst_new(lit) (nnc_3a_addr) { .kind = ADDR_ICONST, .exact.iconst = *lit }
#define nnc_3a_fconst_new(lit) (nnc_3a_addr) { .kind = ADDR_FCONST, .exact.iconst = *lit }

typedef struct _nnc_3a_addr {
    nnc_3a_addr_kind kind;
    union _nnc_3a_addr_exact {
        nnc_3a_cgt cgt;
        nnc_3a_name name;
        nnc_3a_iconst iconst;
        nnc_3a_fconst fconst;
    } exact;
} nnc_3a_addr;

#define nnc_3a_quad_new(opv, resv, ...) (nnc_3a_quad){ .op=opv, .res=resv, __VA_ARGS__ }

typedef struct _nnc_3a_quad {
    nnc_3a_op_kind op;
    nnc_3a_addr res;
    nnc_3a_addr arg1;
    nnc_3a_addr arg2;
} nnc_3a_quad;

extern nnc_3a_quad* quads;
extern nnc_3a_cgt_cnt cgt_cnt;

void nnc_expr_to_3a(const nnc_expression* expr, const nnc_st* st);
void nnc_dump_3a(FILE* to, const nnc_3a_quad* quads);

#endif