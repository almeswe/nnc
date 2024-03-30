#ifndef __NNC_x86_64_GEN_REG_ALLOC2_H__
#define __NNC_x86_64_GEN_REG_ALLOC2_H__

#include "nnc_3a.h"

#define LOCATION_MEM (LOCATION_DATA | LOCATION_LOCAL_STACK | LOCATION_PARAM_STACK)
#define IS_LOCATION_MEM(x) ((x & LOCATION_MEM) > 0)

typedef enum _nnc_location_type {
    LOCATION_NONE        = 0b00001,
    LOCATION_REG         = 0b00010,
    LOCATION_DATA        = 0b00100,
    LOCATION_LOCAL_STACK = 0b01000,
    LOCATION_PARAM_STACK = 0b10000
} nnc_loc_type, nnc_location_type;

typedef nnc_i64 nnc_memory;

typedef enum _nnc_x86_64_register {
    /* General purpose x86_64 (used) registers */
    R_RAX, R_RBX,
    R_RCX, R_RDX,
    R_RSI, R_RDI,
    R_R8,  R_R9,
    R_R10, R_R11,
    R_R12, R_R13,
    R_R14, R_R15,
    /* SIMD extension registers */
    R_XMM0, R_XMM1, R_XMM2, R_XMM3,
    R_XMM4, R_XMM5, R_XMM6, R_XMM7
} nnc_register;

typedef struct _nnc_location {
    nnc_memory mem;    
    nnc_register reg;
    nnc_loc_type where;
} nnc_loc, nnc_location;

typedef struct _nnc_reg_alloc_state {
    // current index in general purpose register table.
    nnc_u8 alloc_idx;
    // current index in SIMD register table.
    nnc_u8 alloc_simd_idx;
    // pushed registers that must be restored at the end of a call
    nnc_register* pushed;
    // amount of stack usage that must be freed after call
    nnc_u32 used_on_stack;
} nnc_reg_alloc_state;

extern nnc_3a_unit* glob_current_unit;
extern const char* glob_register_str[];
extern nnc_reg_alloc_state glob_reg_alloc_state;

void nnc_store_at(
    const nnc_loc* loc,
    const nnc_3a_addr* addr
);

nnc_loc nnc_store_imm(
    const nnc_3a_addr* imm
);

nnc_loc nnc_store_arg(
    const nnc_3a_addr* arg,
    nnc_bool* need_to_push
);

nnc_loc nnc_store_local(
    const nnc_3a_addr* local
);

const nnc_loc* nnc_get_loc(
    const nnc_3a_addr* addr
);

nnc_bool nnc_reserve_reg(
    nnc_register reg
);

void nnc_unreserve_reg(
    nnc_register reg
);

#endif 