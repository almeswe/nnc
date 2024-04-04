#ifndef __NNC_x86_64_GEN_REG_ALLOC_H__
#define __NNC_x86_64_GEN_REG_ALLOC_H__

#include "nnc_3a.h"

#define LOCATION_MEM (L_DATA | L_LOCAL_STACK | L_PARAM_STACK)
#define IS_LOCATION_MEM(x) ((x & LOCATION_MEM) > 0)

typedef enum _nnc_location_type {
    L_NONE        = 0b000001,
    L_REG         = 0b000010,
    L_DATA        = 0b000100,
    L_LOCAL_STACK = 0b001000,
    L_PARAM_STACK = 0b010000,
    L_IMM         = 0b100000
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
    nnc_memory offset;
    nnc_register reg;
    char* ds_name;
    nnc_loc_type where;
    nnc_u64 imm;
    const nnc_type* type;
} nnc_loc, nnc_location;

typedef struct _nnc_call_stack_state {
    nnc_u8 alloc_idx;
    nnc_u8 alloc_simd_idx;
    vector(nnc_register) pushed;
    nnc_u32 offset;
    nnc_bool rax_pushed;
    nnc_bool xmm0_pushed;
} nnc_call_stack_state;

#define glob_current_call_state (&buf_last(glob_call_stack))

extern vector(nnc_call_stack_state) glob_call_stack;

extern nnc_3a_proc* glob_current_unit;
extern const char* glob_reg_str[];

void nnc_call_stack_state_init(
    const nnc_register* do_not_touch
);

void nnc_call_stack_state_next(
    const nnc_3a_addr* param
);

void nnc_call_stack_state_fini();

void nnc_store_at(
    nnc_loc loc,
    const nnc_3a_addr* addr
);

//nnc_loc nnc_store_imm(
//    const nnc_3a_addr* imm
//);

nnc_loc nnc_store_arg(
    const nnc_3a_addr* arg,
    nnc_bool* need_to_push
);

nnc_loc nnc_store_param(
    const nnc_3a_addr* param
);

//nnc_loc nnc_store_local(
//    const nnc_3a_addr* local
//);
//
//nnc_loc nnc_store_global(
//    const nnc_3a_addr* global
//);

const nnc_loc* nnc_get_loc(
    const nnc_3a_addr* addr
);

nnc_bool nnc_reg_in_use(
    nnc_register reg
);

void nnc_push_reg(
    nnc_register reg
);

nnc_bool nnc_reserve_reg(
    nnc_register reg
);

void nnc_unreserve_reg(
    nnc_register reg
);

nnc_loc nnc_store(
    const nnc_3a_addr* addr,
    nnc_bool globally
);

#endif 