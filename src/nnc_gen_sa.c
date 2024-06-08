#include "nnc_state.h"
#include "nnc_gen_sa.h"

#define LOC_LABEL_SIZE 512

/**
 * @brief Priority array for allocation SSE registers.
 *  Used in all cases of SSE allocation.
 */
const static nnc_reg _sse_p[] = {
    R_XMM0, R_XMM1, R_XMM2, R_XMM3,
    R_XMM4, R_XMM5, R_XMM6, R_XMM7
};

/**
 * @brief Priority array for allocation INTEGER registers
 *  for function parameters & arguments.
 */
const static nnc_reg _int_p_param[] = {
    R_RDI, R_RSI, R_RDX, R_RCX, R_R8, R_R9
};

/**
 * @brief Priority array for allocation INTEGER registers
 *  for local CGTs (Compiler Generated Temporaries).
 */
const static nnc_reg _int_p_local[] = {
    R_RAX, R_RBX, R_R10, R_R11, R_R12, 
    R_R13, R_R14, R_R15, R_R9,  R_R8, 
    R_RCX, R_RDX, R_RDI, R_RSI 
};

/**
 * @brief Global vector for storing live ranges for each register.
 * //todo: extend for memory locations, not only for registers.
 */
nnc_static vector(nnc_3a_lr*) glob_lr_vec[21] = {0};

/**
 * @brief Global map for storing location associated with addresses.
 *  `nnc_store` uses it to avoid allocation of new location for same address.
 */
nnc_static _map_(const char*, const nnc_loc*) glob_loc_map = NULL;

/**
 * @brief Global vector of register string representations.
 */
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

/**
 * @brief Checks if two live ranges intersect.
 * @param lr First live range.
 * @param with Second live range.
 * @return Intersection orccured or not.
 */
nnc_static nnc_bool nnc_lr_intersects(const nnc_3a_lr* lr, const nnc_3a_lr* with) {
    if (lr == NULL || with == NULL) {
        return false;
    }
    return (lr->starts <= with->ends && with->starts <= lr->ends);
}

nnc_static nnc_bool nnc_reg_lr_empty(nnc_reg reg) {
    vector(nnc_3a_lr*) lr = glob_lr_vec[reg];
    return lr == NULL || buf_len(lr) == 0;
}

nnc_static nnc_3a_lr* nnc_peek_reg(nnc_reg reg) {
    vector(nnc_3a_lr*) lr = glob_lr_vec[reg];
    if (nnc_reg_lr_empty(reg)) {
        return NULL;
    }
    assert(lr != NULL);
    return buf_last(lr);
}

nnc_static nnc_bool nnc_reg_in_use(nnc_reg reg) {
    nnc_3a_lr* lr = nnc_new(nnc_3a_lr);
    lr->starts = glob_asm_proc->quad_pointer;
    lr->ends = UINT32_MAX;
    return nnc_lr_intersects(lr, nnc_peek_reg(reg));
}

nnc_static void nnc_update_reg(nnc_reg reg, nnc_3a_lr* by) {
    vector(nnc_3a_lr*)* lr = &glob_lr_vec[reg];
    if (nnc_reg_lr_empty(reg)) {
        buf_add(*lr, by);
    }
    else {
        (*lr)[buf_len(*lr) - 1] = by;
    }
}

void nnc_push_reg(nnc_reg reg) {
    vector(nnc_3a_lr*) lr = glob_lr_vec[reg];
    assert(buf_last(lr) != NULL);
    buf_add(lr, NULL);
}

nnc_bool nnc_reserve_reg(nnc_reg reg) {
    //todo: free this lr?
    nnc_3a_lr* lr = nnc_new(nnc_3a_lr);
    lr->starts = glob_asm_proc->quad_pointer;
    lr->ends = UINT32_MAX;
    nnc_bool pushed = false;
    if (nnc_reg_in_use(reg)) {
        nnc_push_reg(reg);
        pushed = true;
    }    
    nnc_update_reg(reg, lr);
    return pushed;
}

void nnc_unreserve_reg(nnc_reg reg) {
    assert(!nnc_reg_lr_empty(reg));
    //assert(nnc_reg_in_use(reg));
    buf_pop(glob_lr_vec[reg]);
}

/**
 * @brief Retrieves live range for specified address.
 * @param addr Address for which to get live range.
 * @return Pointer to live range.
 */
nnc_static nnc_3a_lr* nnc_get_lr(const nnc_3a_addr* addr) {
    nnc_bool addr_has_lr = true;
    const nnc_3a_proc* proc = glob_state.curr.asm_proc.code;
    switch (addr->kind) {
        case ADDR_CGT:  addr_has_lr = map_has(proc->lr_cgt, addr->exact.cgt); break;
        case ADDR_NAME: addr_has_lr = map_has_s(proc->lr_var, addr->exact.name.name); break;
        default: {
            nnc_abort_no_ctx("bad addr kind\n");
        }
    }
    if (addr_has_lr) {
        switch (addr->kind) {
            case ADDR_CGT:  return (nnc_3a_lr*)map_get(proc->lr_cgt, addr->exact.cgt);
            case ADDR_NAME: return (nnc_3a_lr*)map_get_s(proc->lr_var, addr->exact.name.name);
            default: {
                nnc_abort_no_ctx("bad addr kind\n");
            }
        }
    }
    return NULL;
}

/**
 * @brief Creates unique label for CGT.
 * @param buf Buffer of size `LOC_LABEL_SIZE` where label will be stored.
 * @param addr CGT.
 */
nnc_static void nnc_make_cgt_loc_label(char* buf, const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_CGT);
    sprintf(buf, "c_%lu", addr->exact.cgt);
}

/**
 * @brief Creates unique label for global or local variable.
 * @param buf Buffer of size `LOC_LABEL_SIZE` where label will be stored.
 * @param addr Variable.
 */
nnc_static void nnc_make_var_loc_label(char* buf, const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_NAME);
    sprintf(buf, "%s", addr->exact.name.name);
    //sprintf(buf, "v_%s", addr->exact.name.name);
}

/**
 * @brief Creates unique label for string literal.
 * @param buf Buffer of size `LOC_LABEL_SIZE` where label will be stored.
 * @param addr String literal.
 */
nnc_static void nnc_make_str_loc_label(char* buf, const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_SCONST);
    static nnc_u32 str_label_counter = 0;
    //todo: free this somehow?
    static _map_(char*, char*) str_id_value_map = NULL;
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

/**
 * @brief Creates unique label for the address to store it in map.
 * @param buf Buffer of size `LOC_LABEL_SIZE` where label will be stored.
 * @param addr Address for which to create label.
 */
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

/**
 * @brief Gets associated location with some address.
 * @param addr Address for which to search.
 * @return Some location or `NULL`.
 */
const nnc_loc* nnc_get_loc(const nnc_3a_addr* addr) {
    char internal_buf[LOC_LABEL_SIZE] = {0};
    nnc_make_loc_label(internal_buf, addr);
    if (glob_loc_map == NULL) {
        glob_loc_map = map_init();
    }
    assert(glob_loc_map != NULL);
    if (!map_has_s(glob_loc_map, internal_buf)) {
        return NULL;
    }
    return (nnc_loc*)map_get_s(glob_loc_map, internal_buf);
}

/**
 * @brief Puts location associated with some address.
 * @param loc Location to be stored in map.
 * @param addr Address which will be associated.
 */
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

nnc_static nnc_loc* nnc_make_reg_loc(const nnc_reg reg, const nnc_3a_addr* addr) {
    nnc_loc* loc = nnc_new(nnc_loc);
    loc->where = L_REG;
    loc->type = addr->type;
    loc->exact.reg = reg;
    nnc_put_loc(loc, addr);
    return loc;
}

nnc_static nnc_loc* nnc_make_ds_loc(const nnc_3a_addr* addr) {
    char internal_buf[LOC_LABEL_SIZE] = {0};
    nnc_make_loc_label(internal_buf, addr);
    nnc_loc* loc = nnc_new(nnc_loc);
    loc->where = L_DATA;
    loc->type = addr->type;
    loc->exact.dsl = nnc_strdup(internal_buf);
    nnc_put_loc(loc, addr);
    return loc;
}

nnc_static nnc_loc* nnc_make_ss_loc(const nnc_3a_addr* addr) {
    nnc_loc* loc = nnc_new(nnc_loc);
    glob_asm_proc->local_stack_offset += nnc_sizeof(addr->type);
    loc->where = L_STACK;
    loc->type = addr->type;
    loc->exact.mem = -glob_asm_proc->local_stack_offset;
    nnc_put_loc(loc, addr);
    return loc;
}

/**
 * @brief Gets parameter class of the parameter's type.
 *  Can be used for generating function call and storing function
 *  parameters inside callee function. 
 * @param p_type Type of the parameter.
 * @param state State used accross different `nnc_get_pclass` calls.
 * @return Class of the parameter.
 */
nnc_pclass nnc_get_pclass(const nnc_type* p_type, nnc_pclass_state* state) {
    assert(state != NULL);
    nnc_type* u_type = nnc_unalias(p_type);
    if (!state->was_init) {
        state->was_init = true;
        state->int_iter = 0;
        state->sse_iter = 0;
    }
    assert(!nnc_real_type(u_type));
    assert(!nnc_incomplete_type(u_type));
    assert(!nnc_struct_or_union_type(u_type));
    if (state->int_iter < nnc_arr_size(_int_p_param)) {
        state->int_iter += 1;
        return C_INTEGER;
    }
    return C_MEMORY;
}

/**
 * @brief Stores CGT in some location.
 * @param addr Address to be stored.
 * @return Location.
 */
nnc_loc nnc_store_cgt(const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_CGT);
    assert(!nnc_real_type(addr->type));
    nnc_loc* loc = NULL;
    const vector(nnc_reg) p_vec = _int_p_local;
    nnc_u64 p_len = nnc_arr_size(_int_p_local);
    nnc_3a_lr* lr = nnc_get_lr(addr);
    for (nnc_u64 i = 0; i < p_len; i++) {
        assert(p_vec[i] < nnc_arr_size(glob_lr_vec));
        // get mapping record (register => live range associated with it)
        const nnc_3a_lr* top_lr = nnc_peek_reg(p_vec[i]);
        // if current register's live range does not intersects
        // with specified live range, allocate this register
        if (!nnc_lr_intersects(lr, top_lr)) {
            nnc_update_reg(p_vec[i], lr);
            loc = nnc_make_reg_loc(p_vec[i], addr);
            break;
        }
    }
    // if loc is NULL it means it was not allocated in register,
    // so it must be allocated on stack.
    if (loc == NULL) {
        loc = nnc_make_ss_loc(addr);
    }
    assert(loc != NULL);
    return *loc;
}

/**
 * @brief Stores variable in some location.
 *  Can store both local & global variables.
 * @param addr Address to be stored.
 * @return Location.
 */
nnc_loc nnc_store_var(const nnc_3a_addr* addr) {
    assert(addr && addr->kind == ADDR_NAME);
    nnc_loc* loc = NULL;
    nnc_ident_ctx ictx = addr->exact.name.ictx;
    if (ictx == IDENT_GLOBAL ||
        ictx == IDENT_FUNCTION) {
        loc = nnc_make_ds_loc(addr);
        if (ictx == IDENT_FUNCTION) {
            loc->has_offset = true;
        }
    }
    else {
        loc = nnc_make_ss_loc(addr);
    }
    assert(loc != NULL);
    return *loc;
}

/**
 * @brief Stores string literal in data segment.
 *  Same strings are stored under the same label.
 * @param addr Address to be stored.
 * @return Location.
 */
nnc_loc nnc_store_str(const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_SCONST);
    nnc_loc* loc = nnc_make_ds_loc(addr);
    loc->has_offset = true;
    return *loc;
}

/**
 * @brief Stores any kind of immediate value.
 * //todo: float support.
 * @param addr Address to be stored.
 * @return Location.
 */
nnc_loc nnc_store_imm(const nnc_3a_addr* addr) {
    assert(
        addr->kind == ADDR_ICONST ||
        addr->kind == ADDR_FCONST
    );
    if (addr->kind == ADDR_FCONST) {
        assert(addr->type->kind == T_PRIMITIVE_F64);
        assert(false);
    }
    nnc_u64 imm_val = addr->exact.iconst.iconst;
    nnc_loc imm_loc = { 
        .where = L_IMM,
        .exact.imm.u = imm_val
    };
    return imm_loc;
}

/**
 * @brief Stores any kind of address in some location.
 * @param addr Address to be stored.
 * @return Location. If `.where` is L_NONE, nothing happened.
 *  (but see the implementation, may be this is a bug)
 */
nnc_loc nnc_store(const nnc_3a_addr* addr) {
    assert(addr != NULL);
    const nnc_loc* loc = nnc_get_loc(addr);
    // do not allocate new location if it is already allocated
    if (loc != NULL) {
        return *loc;
    } 
    switch (addr->kind) {
        case ADDR_CGT:    return nnc_store_cgt(addr);
        case ADDR_NAME:   return nnc_store_var(addr);
        case ADDR_SCONST: return nnc_store_str(addr);
        case ADDR_FCONST: return nnc_store_imm(addr);
        case ADDR_ICONST: return nnc_store_imm(addr);
        default: {
            nnc_abort_no_ctx("nnc_store: unimplemented addr->kind.\n");
        }
    }
    return (nnc_loc){ .where = L_NONE };
}