#ifndef __NNC_THREE_ADDRESS_CODE_H__
#define __NNC_THREE_ADDRESS_CODE_H__

#define _NNC_ENABLE_PASS_LOGGING 0
#define _NNC_ENABLE_OPTIMIZATIONS 0

#include "nnc_typecheck.h"

#define OP_UNARY    \
         OP_PLUS:   \
    case OP_MINUS:  \
    case OP_BW_NOT

#define OP_UNARY_JUMP \
         OP_CJUMPF:   \
    case OP_CJUMPT

#define OP_BINARY   \
         OP_ADD:    \
    case OP_SUB:    \
    case OP_MUL:    \
    case OP_DIV:    \
    case OP_MOD:    \
    case OP_SHR:    \
    case OP_SHL:    \
    case OP_SAL:    \
    case OP_SAR:    \
    case OP_BW_OR:  \
    case OP_BW_AND: \
    case OP_BW_XOR

#define OP_BINARY_JUMP   \
         OP_CJUMPLT:     \
    case OP_CJUMPGT:     \
    case OP_CJUMPLTE:    \
    case OP_CJUMPGTE:    \
    case OP_CJUMPE:      \
    case OP_CJUMPNE

typedef nnc_u64 nnc_3a_cgt_cnt;
typedef nnc_u32 nnc_3a_label_cnt;

typedef enum _nnc_3a_peep_pattern {
    /* Substitution optimizations */
    OPT_REF_SUBST,
    OPT_COPY_UNARY_SUBST,
    OPT_COPY_UNARY_EX_SUBST,
    OPT_COPY_BINARY_SUBST,
    OPT_COPY_BINARY_L_SUBST,
    OPT_COPY_BINARY_R_SUBST,
    /* Folding optimizations */
    OPT_INDEX_CONST_FOLD,
    OPT_UNARY_CONST_FOLD,
    OPT_BINARY_CONST_FOLD,
    /* Algebraic optimizations */
    OPT_ALG_MUL_ONE,
    OPT_ALG_MUL_ZERO,
    OPT_ALG_ADD_ZERO,
    OPT_ALG_MUL_POW_TWO,
    OPT_ALG_MOD_POW_TWO,
    /* No optimization */
    OPT_NONE,
} nnc_3a_peep_pattern;

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
    OP_SAL,
    OP_SAR,
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

typedef union _nnc_3a_iconst {
    nnc_u64 iconst;
    nnc_i64 sconst;
} nnc_3a_iconst;

#define nnc_3a_mkf1(x) (nnc_3a_addr){ \
    .kind = ADDR_FCONST,              \
    .type = x->type,                  \
    .exact.fconst.fconst = x->exact   \
}

#define nnc_3a_mkf2(x, t) (nnc_3a_addr){ \
    .kind = ADDR_FCONST,                 \
    .type = t,                           \
    .exact.fconst.fconst = x             \
}

typedef struct _nnc_3a_fconst {
    nnc_f64 fconst;
} nnc_3a_fconst;

#define nnc_3a_mks1(x) (nnc_3a_addr){ \
    .kind = ADDR_SCONST,              \
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

//#define STORAGE_MEM (STORAGE_DATA_SEG | STORAGE_STACK_SEG)
//
//typedef enum _nnc_3a_storage_kind {
//    STORAGE_NONE      = 0xb00001,
//    STORAGE_REG       = 0xb00010,
//    STORAGE_DATA_SEG  = 0xb00100,
//    STORAGE_STACK_SEG = 0xb01000,
//} nnc_3a_storage_type;

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

typedef struct _nnc_3a_opt_stat {
    nnc_u64 reduced;
    nnc_f32 percent;
    nnc_u64 initial;
} nnc_3a_opt_stat;

#define nnc_3a_mkblock(x) (nnc_3a_basic){\
    .id = x, .quads = NULL \
}

//typedef struct _nnc_3a_storage {
//    nnc_u32 reg;
//    nnc_u32 mem_offset;
//    nnc_3a_storage_type where;
//} nnc_3a_storage;

typedef struct _nnc_3a_live_range {
    nnc_u32 starts, ends;
    nnc_heap_ptr loc;
} nnc_3a_live_range, nnc_3a_lr;

typedef struct _nnc_3a_basic {
    nnc_u32 id;
    nnc_3a_quad* quads;
} nnc_3a_basic;

#define nnc_3a_node_label(x) x->block->quads[0].label
#define nnc_3a_jump_label(x) (nnc_u32)x->res.exact.iconst.iconst

typedef struct _nnc_3a_cfg_node {
    nnc_u32 id: 30;
    nnc_bool labeled: 1;
    nnc_bool unreachable: 1;
    nnc_3a_basic* block;
    struct _nnc_3a_cfg_node* next;
    struct _nnc_3a_cfg_node* jump;
} nnc_3a_cfg_node;

typedef struct _nnc_3a_cfg {
    nnc_3a_cfg_node* root;
    _vec_(nnc_3a_cfg_node*) nodes;
} nnc_3a_cfg;

typedef struct _nnc_3a_unit {
    const char* name;
    nnc_3a_quad* quads;
    nnc_3a_cfg cfg;
    nnc_3a_opt_stat stat;
    _map_(nnc_3a_cgt,  nnc_3a_lr*) lr_cgt;
    _map_(const char*, nnc_3a_lr*) lr_var;
    nnc_u64 quad_pointer;
    nnc_u32 param_stack_offset;
    nnc_u32 local_stack_offset;
} nnc_3a_unit;

typedef nnc_3a_unit* nnc_3a_code;
typedef nnc_3a_unit  nnc_3a_data;

extern nnc_3a_code code;
extern nnc_3a_data data;
extern nnc_3a_cgt_cnt cgt_cnt;

void nnc_expr_to_3a(
    const nnc_expression* expr,
    const nnc_st* st
);

void nnc_stmt_to_3a(
    const nnc_statement* stmt,
    const nnc_st* st
);

void nnc_ast_to_3a(
    const nnc_ast* ast,
    const nnc_st* st
);

void nnc_dump_3a_code(
    FILE* to,
    const nnc_3a_code code
);

void nnc_dump_3a_data(
    FILE* to,
    const nnc_3a_data data
);

void nnc_dump_3a_code_cfg(
    FILE* to,
    const nnc_3a_code code
);

void nnc_dump_3a_data_cfg(
    FILE* to,
    const nnc_3a_data data
);

_vec_(nnc_3a_quad) nnc_3a_optimize(
    _vec_(nnc_3a_quad) quads,
    nnc_3a_opt_stat* stat
);

nnc_3a_code nnc_3a_optimize_code(
    nnc_3a_code code
);

nnc_3a_data nnc_3a_optimize_data(
    nnc_3a_data data
);

_vec_(nnc_3a_basic) nnc_3a_get_blocks(
    const nnc_3a_unit* unit
);

nnc_3a_cfg nnc_3a_get_cfg(
    _vec_(nnc_3a_basic) blocks
);

nnc_3a_cfg nnc_3a_cfg_optimize(
    nnc_3a_cfg cfg,
    nnc_3a_opt_stat* stat
);

void nnc_dump_3a_cfg(
    const char* name,
    const nnc_3a_cfg* cfg
);

#endif