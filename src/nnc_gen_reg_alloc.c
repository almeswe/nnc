#include "nnc_gen.h"

nnc_static nnc_3a_lr nnc_inf_lr = {
    .starts = 0,
    .ends   = UINT32_MAX
};

const char* nnc_asm_reg_str[] = {
    [R_RAX]  = "rax", 
    [R_RBX]  = "rbx",
    [R_RCX]  = "rcx",
    [R_RDX]  = "rdx",
    [R_RSI]  = "rsi",
    [R_RDI]  = "rdi",
    [R_R8]   = "r8",
    [R_R9]   = "r9",
    [R_R10]  = "r10",
    [R_R11]  = "r11",
    [R_R12]  = "r12",
    [R_R13]  = "r13",
    [R_R14]  = "r14",
    [R_R15]  = "r15",
    [R_XMM0] = "xmm0",
    [R_XMM1] = "xmm1",
    [R_XMM2] = "xmm2",
    [R_XMM3] = "xmm3",
    [R_XMM4] = "xmm4",
    [R_XMM5] = "xmm5",
    [R_XMM6] = "xmm6",
    [R_XMM7] = "xmm7"
};

nnc_static nnc_asm_reg_lr_rec nnc_reg_lr_map[] = {
    [R_RAX]  = { NULL },
    [R_RBX]  = { NULL },
    [R_R10]  = { NULL },
    [R_R11]  = { NULL },
    [R_R12]  = { NULL },
    [R_R13]  = { NULL },
    [R_R14]  = { NULL },
    [R_R15]  = { NULL },
    [R_R9]   = { NULL },
    [R_R8]   = { NULL },
    [R_RCX]  = { NULL },
    [R_RDX]  = { NULL },
    [R_RDI]  = { NULL },
    [R_RSI]  = { NULL },
    [R_XMM0] = { NULL },
    [R_XMM1] = { NULL },
    [R_XMM2] = { NULL },
    [R_XMM3] = { NULL },
    [R_XMM4] = { NULL },
    [R_XMM5] = { NULL },
    [R_XMM6] = { NULL },
    [R_XMM7] = { NULL },
};

nnc_static nnc_3a_lr* nnc_get_lr(nnc_3a_unit* unit, const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:  return (nnc_3a_lr*)map_get(unit->lr_cgt, addr->exact.cgt); 
        case ADDR_NAME: return (nnc_3a_lr*)map_get_s(unit->lr_var, addr->exact.name.name);
        default: nnc_abort_no_ctx("bad addr kind\n");
    }
    return NULL;
}

nnc_static nnc_bool nnc_3a_lr_intersects(const nnc_3a_lr* lr, const nnc_3a_lr* with) {
    if (lr == NULL || with == NULL) {
        return false;
    }
    return (lr->starts <= with->ends && with->starts <= lr->ends);
}

nnc_static nnc_bool nnc_3a_addr_cmp(const nnc_3a_addr* addr, const nnc_3a_addr* with) {
    if (addr == NULL || with == NULL) {
        return false;
    }
    if (addr->kind != with->kind) {
        return false;
    }
    switch (addr->kind) {
        case ADDR_CGT:  return addr->exact.cgt == with->exact.cgt;
        case ADDR_NAME: return strcmp(addr->exact.name.name, with->exact.name.name) == 0;
        default: nnc_abort_no_ctx("bad addr kind\n");
    }
    return false;
}

nnc_static const nnc_3a_storage* nnc_store_in_reg(nnc_3a_lr* lr, nnc_asm_reg reg) {
    lr->storage.reg = (nnc_u32)reg;
    lr->storage.where = STORAGE_REG;
    return &lr->storage;
}

nnc_static const nnc_3a_storage* nnc_store_local_in_mem(nnc_3a_lr* lr, const nnc_3a_addr* local) {
    //todo: alignment?
    lr->storage.mem_offset = nnc_glob_stack_offset;
    lr->storage.where = STORAGE_STACK_SEG;
    nnc_glob_stack_offset += local->type->size;
    return &lr->storage; 
}

nnc_static const nnc_3a_storage* nnc_store_param_in_mem(nnc_3a_lr* lr, const nnc_3a_addr* local) {
    lr->storage.mem_offset = nnc_glob_param_stack_offset;
    lr->storage.where = STORAGE_STACK_SEG;
    nnc_glob_param_stack_offset += local->type->size;
    return &lr->storage;
}

typedef const nnc_3a_storage* (*nnc_store_in_mem_fn)(
    nnc_3a_lr*,
    const nnc_3a_addr*
);

typedef struct _nnc_store_params {
    // live range of some CGT or variable
    nnc_3a_lr* lr;
    // CGT or variable itself
    const nnc_3a_addr* generic;
    // priority of register allocation
    const nnc_asm_reg* priority;
    nnc_u64 priority_size;
} nnc_store_params;

nnc_static const nnc_3a_storage* nnc_perform_store(nnc_store_params* params, nnc_store_in_mem_fn spill) {
    if (params->generic->kind == ADDR_NAME) {
        return spill(params->lr, params->generic);
    }
    for (nnc_u64 i = 0; i < params->priority_size; i++) {
        // get mapping record (register => live range associated with it)
        nnc_asm_reg_lr_rec* rec = &nnc_reg_lr_map[params->priority[i]];
        // if current register's live range does not intersects
        // with specified live range, allocate this register
        if (nnc_3a_lr_intersects(params->lr, rec->lr)) {
            continue;
        }
        rec->lr = params->lr;
        return nnc_store_in_reg(params->lr, params->priority[i]);
    }
    // if program reaches this point, it means that all registers
    // are in use, so store value in memory instead.
    return spill(params->lr, params->generic);
} 

nnc_static const nnc_3a_storage* nnc_store_local(nnc_3a_lr* lr, const nnc_3a_addr* local) {
    // initialize priority lists
    const nnc_asm_reg gen_p[] = {
        R_RAX, R_RBX, R_R10, R_R11, R_R12, 
        R_R13, R_R14, R_R15, R_R9,  R_R8, 
        R_RCX, R_RDX, R_RDI, R_RSI 
    };
    const nnc_asm_reg flt_p[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    // initialize params
    nnc_store_params params = {
        .lr            = lr,
        .generic       = local,
        .priority      = nnc_real_type(local->type) ? flt_p : gen_p,
        .priority_size = nnc_real_type(local->type) ?
            nnc_arr_size(flt_p) : nnc_arr_size(gen_p)
    };
    // call genetic function for storing
    return nnc_perform_store(&params, nnc_store_local_in_mem);
}

nnc_static const nnc_3a_storage* nnc_store_param(nnc_3a_lr* lr, const nnc_3a_addr* param) {
    // initialize priority lists
    const nnc_asm_reg gen_p[] = {
        R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9
    };
    const nnc_asm_reg flt_p[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    // initialize params
    nnc_store_params params = {
        .lr            = lr,
        .generic       = param,
        .priority      = nnc_real_type(param->type) ? flt_p : gen_p,
        .priority_size = nnc_real_type(param->type) ?
            nnc_arr_size(flt_p) : nnc_arr_size(gen_p)
    };
    // call genetic function for storing
    return nnc_perform_store(&params, nnc_store_param_in_mem);
}

const nnc_3a_storage* nnc_store_generic(nnc_3a_unit* unit,
    const nnc_3a_addr* generic, nnc_store_mode mode) {
    if (generic->kind != ADDR_CGT && generic->kind != ADDR_NAME) {
        nnc_abort_no_ctx("bug detected at `nnc_store_generic`\n");
    }   
    nnc_3a_lr* lr = nnc_get_lr(unit, generic);
    if (lr->storage.where != STORAGE_NONE) {
        return &lr->storage;
    }
    switch (mode) {
        case STORE_LOCAL: return nnc_store_local(lr, generic);
        case STORE_PARAM: return nnc_store_param(lr, generic);
        case STORE_GLOBAL: nnc_abort_no_ctx("nnc_store_generic:"
            " STORE_GLOBAL mode is not implemented.\n");
        default: nnc_abort_no_ctx("nnc_store_generic: unknown mode.\n");
    }
    return NULL;
}

nnc_bool nnc_reg_in_use(nnc_3a_unit* unit, nnc_asm_reg reg) {
    nnc_3a_lr lr = {
        .starts = unit->quad_pointer,
        .ends   = UINT32_MAX
    };
    return nnc_3a_lr_intersects(&lr, nnc_reg_lr_map[reg].lr);
}