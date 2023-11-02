#ifndef _NNC_THREE_ADDRESS_CODE_H
#define _NNC_THREE_ADDRESS_CODE_H

#include "nnc_symtable.h"
#include "nnc_expression.h"
#include "nnc_typecheck.h"

typedef nnc_u64 nnc_3a_cgt_cnt;
typedef nnc_u32 nnc_3a_label_cnt;

typedef enum _nnc_3a_op_kind {
    /* ***************** */
    OP_NONE,
    /*  binary operators */
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_SHR,
    OP_SHL,
    OP_BW_OR,
    OP_BW_AND,
    OP_BW_XOR,
    /*  unary operators  */
    OP_PLUS,
    OP_MINUS,
    OP_BW_NOT,
    /*   jump operators  */
    OP_UJUMP,    // Unconditional JUMP
    OP_CJUMPT,   // Conditional JUMP True      (if x goto L) 
    OP_CJUMPF,   // Conditional JUMP False     (if not x goto L)
    OP_CJUMPLT,  // Conditional JUMP Less Than (if x < y goto L)
    OP_CJUMPGT,  // Conditional JUMP Less Than (if x > y goto L
    OP_CJUMPLTE, // Conditional JUMP Less Than Equal (if x <= y goto L)
    OP_CJUMPGTE, // Conditional JUMP Less Than Equal (if x >= y goto L)
    OP_CJUMPE,   // Conditional JUMP Equals     (if x == y goto L)
    OP_CJUMPNE,  // Conditional JUMP Not Equals (if x != y goto L)
    /*  other operators  */
    OP_COPY,
    OP_CAST,
    OP_ARG,
    OP_PCALL,  // Procedure CALL
    OP_FCALL,  // Function CALL
    OP_INDEX,
    OP_RETP,   // RETurn from Procedure
    OP_RETF,   // RETurn from Function
    OP_REF,
    OP_DEREF,
    OP_DEREF_COPY,
    /* ***************** */
} nnc_3a_op_kind;

typedef enum _nnc_3a_addr_kind {
    ADDR_NONE,
    ADDR_NAME,
    ADDR_SCONST,
    ADDR_ICONST,
    ADDR_FCONST,
    ADDR_CGT
} nnc_3a_addr_kind;

typedef nnc_u64 nnc_3a_cgt;

#define nnc_3a_mki1(x) (nnc_3a_addr){  \
    .kind = ADDR_ICONST,               \
    .type = x->type,                   \
    .exact.iconst.iconst = x->exact.u  \
}

#define nnc_3a_mki2(x, t) (nnc_3a_addr){ \
    .kind = ADDR_ICONST,                 \
    .type = t,                           \
    .exact.iconst.iconst = x             \
}

#define nnc_3a_mki3(x) (nnc_3a_addr){ \
    .kind = ADDR_ICONST,              \
    .type = &u32_type,                \
    .exact.iconst.iconst = x          \
} 

typedef struct _nnc_3a_iconst {
    nnc_u64 iconst;
} nnc_3a_iconst;

#define nnc_3a_mkf1(x) (nnc_3a_addr){ \
    .kind = ADDR_FCONST,              \
    .type = x->type,                  \
    .exact.fconst.fconst = x->exact   \
}

typedef struct _nnc_3a_fconst {
    nnc_f64 fconst;
} nnc_3a_fconst;

#define nnc_3a_mks1(x) (nnc_3a_addr){ \
    .kind = ADDR_SCONST,               \
    .type = x->type,                  \
    .exact.sconst.sconst = x->exact   \
}

typedef struct _nnc_3a_sconst {
    nnc_str sconst;
} nnc_3a_sconst;

#define nnc_3a_mkname1(x) (nnc_3a_addr){ \
    .kind = ADDR_NAME,                   \
    .type = x->type,                     \
    .exact.name.name = x->name,          \
    .exact.name.nesting = x->nesting     \
}

#define nnc_3a_mkname2(x, t) (nnc_3a_addr){ \
    .kind = ADDR_NAME,                      \
    .type = t,                              \
    .exact.name.name = x                    \
}

typedef struct _nnc_3a_name {
    const char* name;
    const nnc_nesting* nesting;
} nnc_3a_name;

#pragma pack(push)
#pragma pack(1)
typedef struct _nnc_3a_addr {
    const nnc_type* type;
    nnc_3a_addr_kind kind: 8;
    union _nnc_3a_addr_exact {
        nnc_3a_cgt cgt;
        nnc_3a_name name;
        nnc_3a_sconst sconst;
        nnc_3a_iconst iconst;
        nnc_3a_fconst fconst;
    } exact;
} nnc_3a_addr;
#pragma pack(pop)

#define nnc_3a_mkquad(opv, resv, ...) (nnc_3a_quad){\
     .op=opv, .res=resv, __VA_ARGS__\
}

#define nnc_3a_mkquad1(opv, resv, rest, ...) (nnc_3a_quad){\
     .op=opv, .res=nnc_3a_mkaddr(resv, rest), __VA_ARGS__  \
}

#define nnc_3a_mklabel() (nnc_3a_quad){\
    .label = ++label_cnt\
}

typedef struct _nnc_3a_quad {
    nnc_u32 label: 26;
    nnc_3a_op_kind op: 6;
    nnc_3a_addr res;
    nnc_3a_addr arg1;
    nnc_3a_addr arg2;
} nnc_3a_quad;

typedef struct _nnc_3a_quad_set {
    const char* name;
    nnc_3a_quad* quads;
} nnc_3a_quad_set;

extern nnc_3a_quad_set* sets;
extern nnc_3a_cgt_cnt cgt_cnt;

typedef struct _nnc_3a_basic {
    nnc_u32 id;
    _vec_(nnc_3a_quad_set) quads;
} nnc_3a_basic;

void nnc_expr_to_3a(const nnc_expression* expr, const nnc_st* st);
void nnc_stmt_to_3a(const nnc_statement* stmt, const nnc_st* st);
void nnc_ast_to_3a(const nnc_ast* ast, const nnc_st* st);
void nnc_dump_3a(FILE* to, const nnc_3a_quad_set* sets);

#endif