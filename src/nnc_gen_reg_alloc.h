#ifndef __NNC_x86_64_GEN_REG_ALLOC_H__
#define __NNC_x86_64_GEN_REG_ALLOC_H__

#include "nnc_arena.h"

#define NNC_ALLOC_REG()     nnc_alloc_reg(false)
#define NNC_ALLOC_XMM_REG() nnc_alloc_reg(true)

typedef enum _nnc_x86_64_asm_reg {
    /* General purpose x86_64 (used) registers */
    R_RAX, R_RBX, R_RCX, R_RDX, R_RSI, R_RDI, R_R8,
    R_R9,  R_R10, R_R11, R_R12, R_R13, R_R14, R_R15,
    /* SIMD extension registers */
    R_XMM0, R_XMM1, R_XMM2, R_XMM3,
    R_XMM4, R_XMM5, R_XMM6, R_XMM7
} nnc_asm_reg;

typedef nnc_u8 nnc_asm_reg_state;

extern nnc_asm_reg_state nnc_asm_regs[];
extern const char* nnc_asm_reg_str[];

nnc_asm_reg nnc_alloc_reg(nnc_bool simd);

#endif