#include "nnc_gen_reg_alloc2.h"

const char* glob_register_str[] = {
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

//typedef struct _nnc_addr_loc {
//    nnc_location loc;
//    const nnc_3a_lr lr;
//    const nnc_3a_addr* addr;
//    const nnc_3a_unit* unit;
//} nnc_addr_loc;

vector(nnc_3a_lr*) nnc_reg_lr[21] = {0};
nnc_static dictionary(const char*, nnc_loc*) glob_loc_map = NULL;

nnc_static void nnc_make_loc_key(char* keybuf, const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:  sprintf(keybuf, "_%s@CGT%lu", glob_current_unit->name, addr->exact.cgt); break;
        case ADDR_NAME: sprintf(keybuf, "_%s@_%s",    glob_current_unit->name, addr->exact.name.name); break;
        default: {
            nnc_abort_no_ctx("nnc_make_loc_key: invalid addr\n");
        }
    }
}

nnc_static void nnc_put_loc(const nnc_loc* loc, const nnc_3a_addr* addr) {
    char keybuf[128] = {0};
    nnc_make_loc_key(keybuf, addr);
    //printf("key: %s\n", keybuf);
    char* key = nnc_strdup(keybuf);
    if (glob_loc_map == NULL) {
        glob_loc_map = map_init();
    }
    map_put_s(glob_loc_map, key, loc);
}

nnc_static nnc_3a_lr* nnc_get_lr(const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:  return (nnc_3a_lr*)map_get(glob_current_unit->lr_cgt, addr->exact.cgt);
        case ADDR_NAME: return (nnc_3a_lr*)map_get_s(glob_current_unit->lr_var, addr->exact.name.name);
        default: {
            nnc_abort_no_ctx("bad addr kind\n");
        }
    }
    return NULL;
}

nnc_static nnc_bool nnc_lr_intersects(const nnc_3a_lr* lr, const nnc_3a_lr* with) {
    if (lr == NULL || with == NULL) {
        return false;
    }
    return (lr->starts <= with->ends && with->starts <= lr->ends);
}

nnc_static nnc_bool nnc_reg_lr_empty(nnc_register reg) {
    vector(nnc_3a_lr*) lr = nnc_reg_lr[reg];
    return lr == NULL || buf_len(lr) == 0;
}

nnc_static nnc_3a_lr* nnc_peek_reg(nnc_register reg) {
    vector(nnc_3a_lr*) lr = nnc_reg_lr[reg];
    if (nnc_reg_lr_empty(reg)) {
        return NULL;
    }
    assert(lr != NULL);
    return buf_last(lr);
}

nnc_static void nnc_push_reg(nnc_register reg) {
    vector(nnc_3a_lr*) lr = nnc_reg_lr[reg];
    assert(buf_last(lr) != NULL);
    buf_add(lr, NULL);
}

nnc_static void nnc_update_reg(nnc_register reg, nnc_3a_lr* by) {
    vector(nnc_3a_lr*)* lr = &nnc_reg_lr[reg];
    if (nnc_reg_lr_empty(reg)) {
        buf_add(*lr, by);
    }
    else {
        *lr[buf_len(*lr) - 1] = by;
    }
}

nnc_static nnc_location nnc_store_inside_reg(const nnc_3a_addr* addr, nnc_register reg) {
    nnc_loc* loc = nnc_new(nnc_location);
    loc->reg = reg, loc->where = LOCATION_REG;
    nnc_put_loc(loc, addr);
    return *loc;
}

nnc_static nnc_location nnc_store_inside_arg_stack(const nnc_3a_addr* addr, nnc_memory amount) {
    nnc_memory mem = glob_current_unit->param_stack_offset;
    if (mem == 0) {
        glob_current_unit->param_stack_offset = mem = 8;
    }
    glob_reg_alloc_state.used_on_stack += amount;
    glob_current_unit->param_stack_offset += amount;
    nnc_loc* loc = nnc_new(nnc_location);
    loc->mem = mem, loc->where = LOCATION_PARAM_STACK;
    nnc_put_loc(loc, addr);
    return *loc;
}

nnc_static nnc_location nnc_store_inside_stack(const nnc_3a_addr* addr, nnc_memory amount) {
    nnc_memory mem = glob_current_unit->local_stack_offset;
    if (mem == 0) {
        glob_current_unit->local_stack_offset = mem = 8;
    }
    glob_current_unit->local_stack_offset += amount;
    nnc_loc* loc = nnc_new(nnc_location);
    loc->mem = mem, loc->where = LOCATION_LOCAL_STACK;
    nnc_put_loc(loc, addr);
    return *loc;
}

const nnc_loc* nnc_get_loc(const nnc_3a_addr* addr) {
    assert(
        addr->kind == ADDR_CGT ||
        addr->kind == ADDR_NAME 
    );
    char keybuf[128] = {0};
    nnc_make_loc_key(keybuf, addr);
    if (glob_loc_map == NULL) {
        glob_loc_map = map_init();
    }
    assert(glob_loc_map != NULL);
    if (!map_has_s(glob_loc_map, keybuf)) {
        return NULL;
    }
    return (nnc_loc*)map_get_s(glob_loc_map, keybuf);
}

void nnc_store_at(const nnc_loc* loc, const nnc_3a_addr* addr) {
    assert(false && "not implemented yet.");
}

nnc_loc nnc_store_imm(const nnc_3a_addr* imm) {
    assert(false && "not implemented yet.");
}

nnc_reg_alloc_state glob_reg_alloc_state = {0};

nnc_static void nnc_reg_alloc_state_next(nnc_u8* alloc_idx) {
    assert(alloc_idx);
    *alloc_idx = *alloc_idx + 1;
}

nnc_loc nnc_store_arg(const nnc_3a_addr* arg, nnc_bool* need_to_push) {
    assert(
        arg->kind == ADDR_NAME || 
        arg->kind == ADDR_CGT
    );
    const static nnc_register genp_priority[] = {
        R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9
    };
    const static nnc_register simd_priority[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    const nnc_register* priority = nnc_real_type(arg->type) ?
        simd_priority : genp_priority;
    const nnc_u64 priority_size = nnc_real_type(arg->type) ?
        nnc_arr_size(simd_priority) : nnc_arr_size(genp_priority);
    nnc_u8* current_idx = nnc_real_type(arg->type) ?
        &glob_reg_alloc_state.alloc_simd_idx : &glob_reg_alloc_state.alloc_idx;
    nnc_register current = priority[*current_idx];
    /**************************************************************/
    assert(!nnc_struct_or_union_type(arg->type));
    *need_to_push = false;
    if (*current_idx >= priority_size) {
        nnc_reg_alloc_state_next(current_idx);
        return nnc_store_inside_arg_stack(arg, arg->type->size);
    }
    if (nnc_reserve_reg(current)) {
        *need_to_push = true;
        buf_add(glob_reg_alloc_state.pushed, current);
    }
    nnc_reg_alloc_state_next(current_idx);
    return nnc_store_inside_reg(arg, current);
    /**************************************************************/
}

nnc_loc nnc_store_local(const nnc_3a_addr* local) {
    assert(
        local->kind == ADDR_NAME ||
        local->kind == ADDR_CGT
    );
    const static nnc_register genp_priority[] = {
        R_RAX, R_RBX, R_R10, R_R11, R_R12, 
        R_R13, R_R14, R_R15, R_R9,  R_R8, 
        R_RCX, R_RDX, R_RDI, R_RSI 
    };
    const static nnc_register simd_priority[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    nnc_loc loc = {0};
    nnc_3a_lr* lr = nnc_get_lr(local);
    //fprintf(stderr, "lr: %u to %u\n", lr->starts+1, lr->ends+1);
    assert(lr != NULL);
    const nnc_register* priority = nnc_real_type(local->type) ?
        simd_priority : genp_priority;
    const nnc_u64 priority_size = nnc_real_type(local->type) ?
        nnc_arr_size(simd_priority) : nnc_arr_size(genp_priority);
    /**************************************************************/
    assert(!nnc_struct_or_union_type(local->type));
    const nnc_loc* stored = nnc_get_loc(local);  
    if (stored != NULL) {
        return *stored;
    }
    if (local->kind == ADDR_NAME) {
        return nnc_store_inside_stack(local, local->type->size);
    }
    for (nnc_u64 i = 0; i < priority_size; i++) {
        assert(priority[i] < nnc_arr_size(nnc_reg_lr));
        // get mapping record (register => live range associated with it)
        nnc_3a_lr* top_lr = nnc_peek_reg(priority[i]);
        // if current register's live range does not intersects
        // with specified live range, allocate this register
        if (nnc_lr_intersects(lr, top_lr)) {
            continue;
        }
        nnc_update_reg(priority[i], lr);
        return nnc_store_inside_reg(local, priority[i]);
    }
    return nnc_store_inside_stack(local, local->type->size);
    /**************************************************************/
}

nnc_bool nnc_reserve_reg(nnc_register reg) {
    //todo: free this lr?
    nnc_3a_lr* lr = nnc_new(nnc_3a_lr);
    lr->starts = glob_current_unit->quad_pointer;
    lr->ends = UINT32_MAX;
    if (nnc_lr_intersects(lr, nnc_peek_reg(reg))) {
        nnc_push_reg(reg), nnc_update_reg(reg, lr);
        return true;
    }    
    return false;
}

void nnc_unreserve_reg(nnc_register reg) {
    assert(!nnc_reg_lr_empty(reg));
    buf_pop(nnc_reg_lr[reg]);
}