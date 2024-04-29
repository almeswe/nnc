#include "nnc_gen_reg_alloc.h"

/**
 * @brief Priority array for allocation SSE registers.
 *  Used in all cases of SSE allocation.
 */
const static nnc_register _sse_p[] = {
    R_XMM0, R_XMM1, R_XMM2, R_XMM3,
    R_XMM4, R_XMM5, R_XMM6, R_XMM7
};

/**
 * @brief Priority array for allocation INTEGER registers
 *  for function parameters & arguments.
 */
const static nnc_register _int_p_param[] = {
    R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9
};

/**
 * @brief Priority array for allocation INTEGER registers
 *  for local CGTs (Compiler Generated Temporaries).
 */
const static nnc_register _int_p_local[] = {
    R_RAX, R_RBX, R_R10, R_R11, R_R12, 
    R_R13, R_R14, R_R15, R_R9,  R_R8, 
    R_RCX, R_RDX, R_RDI, R_RSI 
};

#define LOC_LABEL_SIZE 512

#define nnc_get_priority(x)      nnc_real_type((x)->type) ? simd_priority : genp_priority
#define nnc_get_priority_size(x) nnc_real_type((x)->type) ? nnc_arr_size(simd_priority ) : nnc_arr_size(genp_priority) 

vector(nnc_call_stack_state) glob_call_stack = NULL;

nnc_static nnc_u32 glob_s_count  = 0;
nnc_static nnc_u32 glob_ss_count = 0;
nnc_static nnc_u32 glob_sd_count = 0;
nnc_static nnc_u32 glob_imm64_count = 0;

const char* glob_reg_str[] = {
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

vector(nnc_3a_lr*) glob_reg_lr[21] = {0};
dictionary(const char*, nnc_loc*) glob_loc_map = NULL;

__attribute__((unused))
nnc_static void nnc_lr_diagnostics(nnc_map_key key, nnc_map_val val) {
    nnc_loc* loc = (nnc_loc*)val;
    if (loc->where == L_LOCAL_STACK) {
        printf("key: %s, val: %d\n", (char*)key, loc->where);
    }
}

__attribute__((unused))
nnc_static void nnc_lr_print(nnc_register reg) {
    nnc_u64 size = buf_len(glob_reg_lr[reg]);
    fprintf(stdout, " %%%s (size: %lu):\n", glob_reg_str[reg], size);
    for (nnc_u64 i = 0; i < size; i++) {
        const nnc_3a_lr* lr = glob_reg_lr[reg][i];
        if (lr == NULL) {
            fprintf(stdout, " %lu. EMPTY\n", i + 1);
        }
        else {
            fprintf(stdout, " %lu. [%u:%u]\n", i + 1, lr->starts, lr->ends);
        }
    }
    fprintf(stdout, "\n");
}

nnc_static nnc_3a_lr* nnc_get_lr(const nnc_3a_addr* addr) {
    nnc_bool addr_has_lr = true;
    switch (addr->kind) {
        case ADDR_CGT:  addr_has_lr = map_has(glob_current_proc->lr_cgt, addr->exact.cgt); break;
        case ADDR_NAME: addr_has_lr = map_has_s(glob_current_proc->lr_var, addr->exact.name.name); break;
        default: {
            nnc_abort_no_ctx("bad addr kind\n");
        }
    }
    if (addr_has_lr) {
        switch (addr->kind) {
            case ADDR_CGT:  return (nnc_3a_lr*)map_get(glob_current_proc->lr_cgt, addr->exact.cgt);
            case ADDR_NAME: return (nnc_3a_lr*)map_get_s(glob_current_proc->lr_var, addr->exact.name.name);
            default: {
                nnc_abort_no_ctx("bad addr kind\n");
            }
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
    vector(nnc_3a_lr*) lr = glob_reg_lr[reg];
    return lr == NULL || buf_len(lr) == 0;
}

nnc_static nnc_3a_lr* nnc_peek_reg(nnc_register reg) {
    vector(nnc_3a_lr*) lr = glob_reg_lr[reg];
    if (nnc_reg_lr_empty(reg)) {
        return NULL;
    }
    assert(lr != NULL);
    return buf_last(lr);
}

nnc_static nnc_bool nnc_reg_in_use(nnc_register reg) {
    nnc_3a_lr* lr = nnc_new(nnc_3a_lr);
    lr->starts = glob_current_proc->quad_pointer;
    lr->ends = UINT32_MAX;
    return nnc_lr_intersects(lr, nnc_peek_reg(reg));
}

void nnc_push_reg(nnc_register reg) {
    vector(nnc_3a_lr*) lr = glob_reg_lr[reg];
    assert(buf_last(lr) != NULL);
    buf_add(lr, NULL);
}

nnc_static void nnc_update_reg(nnc_register reg, nnc_3a_lr* by) {
    vector(nnc_3a_lr*)* lr = &glob_reg_lr[reg];
    if (nnc_reg_lr_empty(reg)) {
        buf_add(*lr, by);
    }
    else {
        (*lr)[buf_len(*lr) - 1] = by;
    }
}

nnc_bool nnc_reserve_reg(nnc_register reg) {
    //todo: free this lr?
    nnc_3a_lr* lr = nnc_new(nnc_3a_lr);
    lr->starts = glob_current_proc->quad_pointer;
    lr->ends = UINT32_MAX;
    nnc_bool pushed = false;
    if (nnc_reg_in_use(reg)) {
        nnc_push_reg(reg);
        pushed = true;
    }    
    nnc_update_reg(reg, lr);
    return pushed;
}

void nnc_unreserve_reg(nnc_register reg) {
    assert(!nnc_reg_lr_empty(reg));
    //assert(nnc_reg_in_use(reg));
    buf_pop(glob_reg_lr[reg]);
}

nnc_static void nnc_make_cgt_loc_label(char* buf, const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_CGT);
    sprintf(buf, "cgt_%s@%lu", glob_current_proc->name, addr->exact.cgt);
}

nnc_static void nnc_make_var_loc_label(char* buf, const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_NAME);
    sprintf(buf, "v_%s", addr->exact.name.name);
}

nnc_static void nnc_make_str_loc_label(char* buf, const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_SCONST);

    static nnc_u32 str_label_counter = 0;
    //todo: free this somehow?
    static dictionary(char*, char*) str_id_value_map = NULL;

    nnc_str value = addr->exact.sconst.sconst;

    if (str_id_value_map == NULL) {
        str_id_value_map = map_init_with(8);
    }
    if (map_has_s(str_id_value_map, value)) {
        const char* label = map_get_s(str_id_value_map, value);
        strcpy(buf, label);
    }
    else {
        sprintf(buf, "s_%u", str_label_counter++);
        char* label = nnc_strdup(buf);
        map_put_s(str_id_value_map, value, label);
    }
}

nnc_static nnc_bool nnc_make_loc_label(char* buf, const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:    nnc_make_cgt_loc_label(buf, addr); break;
        case ADDR_NAME:   nnc_make_var_loc_label(buf, addr); break;
        case ADDR_SCONST: nnc_make_str_loc_label(buf, addr); break;
        default: {
            return false;
        }
    }
    return true;
}

nnc_static void nnc_put_loc(const nnc_loc* loc, const nnc_3a_addr* addr) {
    char internal_buf[LOC_LABEL_SIZE] = {0};
    nnc_bool res = nnc_make_loc_label(internal_buf, addr);
    if (!res) {
        printf("addr type: %d\n", addr->kind);
        assert(res);
    }
    char* key = nnc_strdup(internal_buf);
    if (glob_loc_map == NULL) {
        glob_loc_map = map_init();
    }
    map_put_s(glob_loc_map, key, loc);
    //todo: it seems that map rehasing causes data loss
    //printf("-- %s::nnc_put_loc::map_put_s %p len: %u cap: %u\n", glob_current_proc->name, glob_loc_map, glob_loc_map->len, glob_loc_map->cap);
}

const nnc_loc* nnc_get_loc(const nnc_3a_addr* addr) {
    char internal_buf[LOC_LABEL_SIZE] = {0};
    nnc_make_loc_label(internal_buf, addr);
    //printf("------- nnc_get_loc: `%s`\n", internal_buf);
    if (glob_loc_map == NULL) {
        glob_loc_map = map_init();
    }
    assert(glob_loc_map != NULL);
    if (!map_has_s(glob_loc_map, internal_buf)) {
        return NULL;
    }
    return (nnc_loc*)map_get_s(glob_loc_map, internal_buf);
}

nnc_static nnc_loc nnc_store_inside_reg(const nnc_3a_addr* addr, nnc_register reg) {
    nnc_loc* loc = nnc_new(nnc_loc);
    loc->type = addr->type;
    loc->reg = reg, loc->where = L_REG;
    nnc_put_loc(loc, addr);
    return *loc;
}

nnc_static nnc_loc nnc_store_arg_on_stack(const nnc_3a_addr* arg, nnc_memory amount) {
    assert(amount > 0);
    glob_current_call_state->offset += amount;
    nnc_u32 offset = glob_current_call_state->offset;
    nnc_loc* loc = nnc_new(nnc_loc);
    loc->type = arg->type;
    loc->offset = offset, loc->where = L_PARAM_STACK;
    nnc_put_loc(loc, arg);
    return *loc;
}
nnc_static nnc_loc nnc_store_param_on_stack(const nnc_3a_addr* param, nnc_memory amount) {
    assert(amount > 0);
    glob_current_call_state->offset += amount;
    nnc_u32 stack_usage = glob_current_proc->stack_usage;
    nnc_u32 offset = glob_current_call_state->offset;
    offset = 16 + stack_usage - offset;
    nnc_loc* loc = nnc_new(nnc_loc);
    loc->type = param->type;
    loc->offset = offset, loc->where = L_PARAM_STACK;
    nnc_put_loc(loc, param);
    return *loc;
}   

nnc_static nnc_loc nnc_store_fn(const nnc_3a_addr* global_fn) {
    char internal_buf[LOC_LABEL_SIZE] = {0};
    sprintf(internal_buf, "_%s", global_fn->exact.name.name);
    nnc_loc* new_loc = nnc_new(nnc_loc);
    new_loc->where = L_DATA;
    new_loc->offset = 1;
    new_loc->type = global_fn->type;
    new_loc->ds_name = nnc_strdup(internal_buf);
    nnc_put_loc(new_loc, global_fn);
    return *new_loc;
}

nnc_static nnc_loc nnc_store_on_data(const nnc_3a_addr* global) {
    char internal_buf[LOC_LABEL_SIZE] = {0};
    // assuming that `nnc_get_loc` check was done already
    nnc_make_loc_label(internal_buf, global);
    nnc_loc* new_loc = nnc_new(nnc_loc);
    new_loc->where = L_DATA;
    new_loc->type = global->type;
    new_loc->offset = global->kind == ADDR_SCONST;
    new_loc->ds_name = nnc_strdup(internal_buf);
    nnc_put_loc(new_loc, global);
    return *new_loc;
}

nnc_static nnc_loc nnc_store_on_stack(const nnc_3a_addr* local, nnc_memory requires) {
    assert(requires > 0);
    glob_current_proc->local_stack_offset += requires;
    nnc_u32 offset = glob_current_proc->local_stack_offset;
    nnc_loc* loc = nnc_new(nnc_loc);
    loc->type = local->type;
    loc->offset = offset, loc->where = L_LOCAL_STACK;
    nnc_put_loc(loc, local);
    return *loc;
}

void nnc_call_stack_state_init(const nnc_register* do_not_touch) {
    nnc_call_stack_state state = {0};
    for (nnc_register r = R_RBX; r <= R_R15; r++) {
        if (do_not_touch == NULL || *do_not_touch != r) {
            if (nnc_reg_in_use(r)) {
                nnc_push_reg(r);
                buf_add(state.pushed, r);
            }
        }
    }
    buf_add(glob_call_stack, state);
}

nnc_u8 nnc_call_stack_state_idx(const nnc_3a_addr* param) {
    assert(param != NULL);
    return nnc_real_type(param->type) ? 
        glob_current_call_state->alloc_simd_idx :
        glob_current_call_state->alloc_idx;
}

void nnc_call_stack_state_next(const nnc_3a_addr* param) {
    nnc_u8* idx = nnc_real_type(param->type) ? 
        &glob_current_call_state->alloc_simd_idx :
        &glob_current_call_state->alloc_idx;
    *idx = *idx + 1;
}

void nnc_call_stack_state_fini() {
    if (glob_current_call_state->pushed != NULL) {
        buf_free(glob_current_call_state->pushed);
    }
    assert(glob_call_stack != NULL);
    buf_pop(glob_call_stack);
}

nnc_static nnc_loc nnc_store_ss_or_sd(const nnc_3a_addr* imm) {
    char internal_buf[128] = {0};
    static nnc_u32 ss_or_sd_counter = 0;
    snprintf(internal_buf, sizeof internal_buf,
        "sd_%u", ++ss_or_sd_counter);
    nnc_loc* new_loc = nnc_new(nnc_loc);
    new_loc->where = L_DATA;
    new_loc->ds_name = nnc_strdup(internal_buf);
    nnc_put_loc(new_loc, imm);
    return *new_loc;
}

nnc_static nnc_loc nnc_store_imm64(const nnc_3a_addr* imm) {
    assert(false);
    char internal_buf[128] = {0};
    static nnc_u32 imm64_counter = 0;
    snprintf(internal_buf, sizeof internal_buf,
        "imm64_%u", ++imm64_counter);
    nnc_loc* new_loc = nnc_new(nnc_loc);
    new_loc->where = L_DATA;
    new_loc->ds_name = nnc_strdup(internal_buf);
    nnc_put_loc(new_loc, imm);
    return *new_loc;
}

nnc_loc nnc_store_imm(const nnc_3a_addr* imm) {
    assert(
        imm->kind == ADDR_ICONST ||
        imm->kind == ADDR_FCONST
    );
    if (imm->kind == ADDR_FCONST) {
        assert(imm->type->kind == T_PRIMITIVE_F64);
        return nnc_store_ss_or_sd(imm);
    }
    nnc_u64 imm_val = imm->exact.iconst.iconst;
    //if (imm->kind == ADDR_ICONST) {
    //    if (imm_val > UINT32_MAX) {
    //        return nnc_store_imm64(imm);
    //    }
    //}
    nnc_loc imm_loc = { .where = L_IMM };
    imm_loc.imm = imm_val;
    return imm_loc;
}

nnc_loc nnc_store_str(const nnc_3a_addr* str) {
    assert(str->kind == ADDR_SCONST);
    return nnc_store_on_data(str);
}

nnc_loc nnc_store_arg(const nnc_3a_addr* arg) {
    assert(
        arg->kind == ADDR_NAME || 
        arg->kind == ADDR_CGT ||
        arg->kind == ADDR_ICONST
    );
    assert(!nnc_struct_or_union_type(arg->type));
    const static nnc_register genp_priority[] = {
        R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9
    };
    const static nnc_register simd_priority[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    const nnc_register* priority = nnc_get_priority(arg);
    const nnc_u64 priority_size = nnc_get_priority_size(arg);
    nnc_u8 current_idx = nnc_call_stack_state_idx(arg);
    nnc_register current = priority[current_idx];
    if (current_idx >= priority_size) {
        return nnc_store_arg_on_stack(arg, nnc_sizeof(arg->type));
    }
    // create live range that ends where appropriate call happens 
    nnc_3a_lr* arg_lr = nnc_new(nnc_3a_lr);
    arg_lr->starts = glob_current_proc->quad_pointer;
    arg_lr->ends = glob_current_call_state->call_ptr;
    nnc_update_reg(current, arg_lr);
    return nnc_store_inside_reg(arg, current);
}

nnc_loc nnc_spill_param(const nnc_3a_addr* param) {
    assert(param->kind == ADDR_NAME);
    nnc_loc* loc = (nnc_loc*)nnc_get_loc(param);
    assert(loc != NULL && loc->where == L_REG);
    nnc_unreserve_reg(loc->reg);
    nnc_dispose(loc);
    return nnc_store_on_stack(param, nnc_sizeof(param->type));
}

nnc_loc nnc_store_param(const nnc_3a_addr* param) {
    assert(param->kind == ADDR_NAME);
    assert(!nnc_struct_or_union_type(param->type));
    nnc_3a_lr* lr = nnc_get_lr(param);
    if (lr == NULL) {
        return (nnc_loc){0};
    }
    const static nnc_register genp_priority[] = {
        R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9
    };
    const static nnc_register simd_priority[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    const nnc_register* priority = nnc_get_priority(param);
    const nnc_u64 priority_size = nnc_get_priority_size(param);
    nnc_u8 current_idx = nnc_call_stack_state_idx(param);
    nnc_register current = priority[current_idx];
    if (current_idx >= priority_size) {
        return nnc_store_param_on_stack(param, nnc_sizeof(param->type));
    }
    nnc_update_reg(current, lr);
    return nnc_store_inside_reg(param, current);
}

nnc_static nnc_loc nnc_store_cgt(const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_CGT);
    const vector(nnc_register) priority = _int_p_local;
    nnc_u64 priority_size = nnc_arr_size(_int_p_local);
    if (nnc_real_type(addr->type)) {
        priority = _sse_p, priority_size = nnc_arr_size(_sse_p);
    }
    nnc_3a_lr* lr = nnc_get_lr(addr);
    assert(lr != NULL);
    assert(!nnc_struct_or_union_type(addr->type));
    for (nnc_u64 i = 0; i < priority_size; i++) {
        assert(priority[i] < nnc_arr_size(glob_reg_lr));
        // get mapping record (register => live range associated with it)
        nnc_3a_lr* top_lr = nnc_peek_reg(priority[i]);
        // if current register's live range does not intersects
        // with specified live range, allocate this register
        if (nnc_lr_intersects(lr, top_lr)) {
            continue;
        }
        nnc_update_reg(priority[i], lr);
        return nnc_store_inside_reg(addr, priority[i]);
    }
    return nnc_store_on_stack(addr, nnc_sizeof(addr->type));
}

nnc_static nnc_loc nnc_store_var(const nnc_3a_addr* addr, nnc_bool globally) {
    assert(addr->kind == ADDR_NAME);
    nnc_ident_ctx ictx = addr->exact.name.ictx;
    if (ictx == IDENT_FUNCTION) {
        return nnc_store_fn(addr);
    }
    if (ictx == IDENT_GLOBAL || globally) {
        return nnc_store_on_data(addr);
    }
    else {
        return nnc_store_on_stack(addr, nnc_sizeof(addr->type));
    }
}

nnc_loc nnc_store(const nnc_3a_addr* addr, nnc_bool globally) {
    const nnc_loc* loc = nnc_get_loc(addr); 
    if (loc != NULL) {
        return *loc;
    }
    //if (addr->kind == ADDR_NAME) {
    //    char internal_buf[512] = {0};
    //    nnc_make_loc_label(internal_buf, addr);
    //    printf("-- %s: allocating var: %s (label: `%s`)\n", glob_current_proc->name, addr->exact.name.name, internal_buf);
    //    //nnc_map_iter(glob_loc_map, diagnostics);
    //}
    switch (addr->kind) {
        case ADDR_CGT:    return nnc_store_cgt(addr);
        case ADDR_SCONST: return nnc_store_str(addr);
        case ADDR_FCONST: return nnc_store_imm(addr);
        case ADDR_ICONST: return nnc_store_imm(addr);
        case ADDR_NAME:   return nnc_store_var(addr, globally);
        default: {
            nnc_abort_no_ctx("nnc_store: unimplemented addr->kind.\n");
        }
    }
    return *loc;
}