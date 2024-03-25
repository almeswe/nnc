#ifndef __NNC_x86_64_GEN_REG_ALLOC_H__
#define __NNC_x86_64_GEN_REG_ALLOC_H__

#include "nnc_arena.h"

extern nnc_u32 nnc_glob_stack_offset;
extern nnc_u32 nnc_glob_param_stack_offset;

typedef enum _nnc_x86_64_asm_reg {
    /* General purpose x86_64 (used) registers */
    R_RAX, R_RBX, R_RCX, R_RDX, R_RSI, R_RDI, R_R8,
    R_R9,  R_R10, R_R11, R_R12, R_R13, R_R14, R_R15,
    /* SIMD extension registers */
    R_XMM0, R_XMM1, R_XMM2, R_XMM3,
    R_XMM4, R_XMM5, R_XMM6, R_XMM7
} nnc_asm_reg;

extern const char* nnc_asm_reg_str[];

typedef struct _nnc_asm_reg_lr_rec {
    const nnc_3a_lr* lr;
} nnc_asm_reg_lr_rec;

typedef enum _nnc_store_mode {
    STORE_LOCAL,
    STORE_PARAM,
    STORE_GLOBAL // todo: ??
} nnc_store_mode;

const nnc_3a_storage* nnc_store_generic(
    nnc_3a_unit* unit,
    const nnc_3a_addr* generic,
    nnc_store_mode mode
);

nnc_bool nnc_reg_in_use(
    nnc_3a_unit* unit,
    nnc_asm_reg reg
);

#endif