#include "nnc_gen.h"

#define REG_HAS_JUNK_0  (0)
#define REG_HAS_JUNK_8  (1)
#define REG_HAS_JUNK_16 (2)
#define REG_HAS_JUNK_32 (4)
#define REG_HAS_JUNK_64 (8)

#define S_QWORD 8
#define S_DWORD 4
#define S_WORD  2
#define S_BYTE  1

#define nnc_mkloc_reg(r, t) (nnc_loc){ .where = L_REG, .reg = r, .type = t }

nnc_assembly_file glob_current_asm_file = {0};
nnc_assembly_proc glob_current_asm_proc = {0};

nnc_3a_proc* glob_current_proc = NULL;

nnc_static nnc_u8 glob_reg_junk_level[21] = {0};

#define glob_current_quad (&glob_current_proc->quads[glob_current_proc->quad_pointer])

#define gen_t0(...)    text_put(__VA_ARGS__)
#define gen_t1(...)    text_put("   " __VA_ARGS__)
#define gen_instr(i)   gen_t1("%s ", nnc_asm_instr_str[i])
#define gen_reg(r)     gen_t0("%s", glob_reg_str[(r)])
#if 1
    #define gen_note(...)  gen_t1("# "  __VA_ARGS__)
#else
    #define gen_note(...)
#endif
#define gen_comma()    gen_t0(", ")
#define gen_crlf()     text_put("\r\n")

#define NNC_LABEL_PREFIX     "label_"
#define NNC_RET_LABEL_PREFIX "label_ret_"

nnc_static void nnc_gen_push_reg(nnc_register reg) {
    nnc_push_reg(reg);
    gen_t1("push %s\n", glob_reg_str[reg]);
}

nnc_static void nnc_pollute(nnc_register reg, nnc_u8 amount) {
    assert(
        amount == 0 ||
        amount == 1 ||
        amount == 2 ||
        amount == 4 ||
        amount == 8
    );
    glob_reg_junk_level[reg] = amount;
}

nnc_static nnc_bool nnc_gen_reserve_reg(nnc_register reg) {
    nnc_bool need_push = nnc_reserve_reg(reg);
    if (need_push) {
        gen_instr(I_PUSH);
        gen_t0("%s\n", glob_reg_str[reg]);
    }
    return need_push;
}

nnc_static void nnc_gen_pop_reg(nnc_register reg, nnc_bool pushed) {
    nnc_unreserve_reg(reg);
    if (pushed) {
        gen_t1("pop %s\n", glob_reg_str[reg]);
        nnc_pollute(reg, REG_HAS_JUNK_64);
    }
}

nnc_static void nnc_gen_unreserve_reg(nnc_register reg) {
    nnc_gen_pop_reg(reg, true);
}

nnc_static void nnc_gen_alloc_stack(nnc_u32 amount) {
    if (amount > 0) {
        gen_t1("sub rsp, %u\n", amount);
    }
}

nnc_static void nnc_gen_dealloc_stack(nnc_u32 amount) {
    if (amount > 0) {
        gen_t1("add rsp, %u\n", amount);
    }
}

nnc_static const char* nnc_get_ptr_size(const nnc_type* type) {
    if (nnc_arr_or_ptr_type(type)) {
        return "qword";
    }
    switch (nnc_sizeof(type)) {
        case S_BYTE:  return "byte";
        case S_WORD:  return "word";
        case S_DWORD: return "dword";
        case S_QWORD: return "qword";
        default: {
            nnc_abort_no_ctx("nnc_get_ptr_size: unsupported size.\n");
        }
    }
    return NULL;
}

nnc_static const char* nnc_get_reg_of_size(nnc_register reg, const nnc_type* type) {
    static const char* arr[][4] = {
        [R_RAX]  = { "al", "ax", "eax", "rax" },
        [R_RBX]  = { "bl", "bx", "ebx", "rbx" },
        [R_RCX]  = { "cl", "cx", "ecx", "rcx" },
        [R_RDX]  = { "dl", "dx", "edx", "rdx" },
        [R_RSI]  = { "sil", "si", "esi", "rsi" },
        [R_RDI]  = { "dil", "di", "edi", "rdi" },
        [R_R8]   = { "r8b", "r8w", "r8d", "r8" },
        [R_R9]   = { "r9b", "r9w", "r9d", "r9" },
        [R_R10]  = { "r10b", "r10w", "r10d", "r10" },
        [R_R11]  = { "r11b", "r11w", "r11d", "r11" },
        [R_R12]  = { "r12b", "r12w", "r12d", "r12" },
        [R_R13]  = { "r13b", "r13w", "r13d", "r13" },
        [R_R14]  = { "r14b", "r14w", "r14d", "r14" },
        [R_R15]  = { "r15b", "r15w", "r15d", "r15" },
        [R_XMM0] = { "xmm0", "xmm0", "xmm0", "xmm0" },
        [R_XMM1] = { "xmm1", "xmm1", "xmm1", "xmm1" },
        [R_XMM2] = { "xmm2", "xmm2", "xmm2", "xmm2" },
        [R_XMM3] = { "xmm3", "xmm3", "xmm3", "xmm3" },
        [R_XMM4] = { "xmm4", "xmm4", "xmm4", "xmm4" },
        [R_XMM5] = { "xmm5", "xmm5", "xmm5", "xmm5" },
        [R_XMM6] = { "xmm6", "xmm6", "xmm6", "xmm6" },
        [R_XMM7] = { "xmm7", "xmm7", "xmm7", "xmm7" }
    };
    if (nnc_arr_or_ptr_type(type)) {
        return arr[reg][3];
    }
    if (nnc_real_type(type)) {
        return arr[reg][0];
    }
    else {
        switch (nnc_sizeof(type)) {
            case S_BYTE:  return arr[reg][0];
            case S_WORD:  return arr[reg][1];
            case S_DWORD: return arr[reg][2];
            case S_QWORD: return arr[reg][3];
            default: {
                nnc_abort_no_ctx("nnc_get_reg_of_size: unsupported size.\n");
            }
        }
    }
    return NULL;
}

nnc_static nnc_bool nnc_addr_inside_reg(const nnc_3a_addr* addr, nnc_register reg) {
    const nnc_loc* loc = nnc_get_loc(addr);
    if (loc && loc->where == L_REG) {
        return loc->reg == reg;
    }
    return false;
}

nnc_static nnc_bool nnc_addr_inside(const nnc_3a_addr* addr, nnc_loc_type at) {
    const nnc_loc* loc = nnc_get_loc(addr);
    return loc && (loc->where & at) != 0;
}

nnc_static void nnc_gen_loc(const nnc_loc* at, const nnc_type* type) {
    switch (at->where) {
        case L_REG: {
            if (at->dereference) {
                const char* ptr_size = nnc_get_ptr_size(type);
                gen_t0("%s ptr [%s]", ptr_size, glob_reg_str[at->reg]);
            }
            else {
                gen_t0("%s", nnc_get_reg_of_size(at->reg, type));
            }
            break;
        }
        case L_LOCAL_STACK: {
            const char* ptr_size = nnc_get_ptr_size(type);
            gen_t0("%s ptr [rbp-%ld]", ptr_size, at->offset);
            break;
        }
        case L_PARAM_STACK: {
            const char* ptr_size = nnc_get_ptr_size(type);
            gen_t0("%s ptr [rbp+%ld]", ptr_size, at->offset);
            break;
        }
        case L_DATA: {
            if (at->offset) {
                gen_t0("offset ");    
            }
            gen_t0("%s", at->ds_name);
            break;
        }
        case L_IMM: {
            gen_t0("0x%lx", at->imm);
            break;
        }
        default: {
            nnc_abort_no_ctx("nnc_gen_storage: unsupported storage type.\n");
        }
    }
}

nnc_static void nnc_gen_xor_reg(const nnc_loc* reg) {
    assert(reg->where == L_REG);
    gen_t1("xor %s, %s\n", glob_reg_str[reg->reg], glob_reg_str[reg->reg]);
    nnc_pollute(reg->reg, REG_HAS_JUNK_0);
}

nnc_static void nnc_gen_clear_reg(const nnc_loc* reg) {
    assert(reg->where == L_REG);
    nnc_u8 junk_level = glob_reg_junk_level[reg->reg];
    nnc_u64 size = nnc_sizeof(reg->type);
    if (nnc_arr_type(reg->type)) {
        size = REG_HAS_JUNK_64;
    }
    if (junk_level != REG_HAS_JUNK_0) {
        if (junk_level > size) {
            gen_note("---- reg %s has %d junked byte(s)\n", 
                glob_reg_str[reg->reg], junk_level);
            nnc_gen_xor_reg(reg);
        }
    }
    nnc_pollute(reg->reg, size);
}

nnc_static void nnc_gen_operand(const nnc_3a_addr* addr, const nnc_type* type) {
    nnc_loc loc = nnc_store(addr, false);
    nnc_gen_loc(&loc, type);
}

nnc_static void nnc_gen_mov_mem_to_reg(const nnc_loc* reg, const nnc_loc* mem) {
    assert(mem->where & LOCATION_MEM);
    gen_note("--- mov memory to reg:\n");
    nnc_asm_instr i_mov = I_MOV;
    nnc_gen_clear_reg(reg);
    if (nnc_arr_type(mem->type)) {
        if (mem->where == L_DATA) {
            ((nnc_loc*)mem)->offset = 1;
        }
        else {
            i_mov = I_LEA;
        }
    }
    else {
        if (nnc_sizeof(mem->type) > nnc_sizeof(reg->type)) {
            i_mov = nnc_signed_type(mem->type) ? I_MOVSX : I_MOVZX;
        }
    }
    gen_instr(i_mov);
    nnc_gen_loc(reg, mem->type);
    gen_comma();
    nnc_gen_loc(mem, mem->type);
    gen_crlf();
}

nnc_static void nnc_gen_reg_to_mem_mov(const nnc_loc* reg, const nnc_loc* mem) {
    assert(reg->where == L_REG);
    assert(mem->where & LOCATION_MEM);
    gen_note("--- mov reg to memory:\n");
    //todo: maybe not regular mov?
    gen_instr(I_MOV);
    nnc_gen_loc(mem, mem->type);
    gen_comma();
    gen_t0("%s", nnc_get_reg_of_size(reg->reg, mem->type));
    gen_crlf();
}

nnc_static void nnc_gen_mov_reg_to_reg(const nnc_loc* dst_reg, const nnc_loc* src_reg) {
    assert(dst_reg->where == L_REG);
    assert(src_reg->where == L_REG);
    gen_note("--- mov reg to reg:\n");
    nnc_asm_instr i_mov = I_MOV;
    const nnc_type* dst_type = dst_reg->type;
    const nnc_type* src_type = src_reg->type;
    if (!dst_reg->dereference) {
        if (src_reg->reg == dst_reg->reg) {
            return;
        }
        nnc_gen_clear_reg(dst_reg);
    }
    if (src_reg->dereference) {
        // if dereferencing pointer to array,
        // just mov, don't need to dereference
        //todo: changing const type...
        if (nnc_arr_type(dst_reg->type)) {
            ((nnc_loc*)src_reg)->dereference = false;
        }
    }
    gen_instr(i_mov);
    nnc_gen_loc(dst_reg, dst_type);
    gen_comma();
    nnc_gen_loc(src_reg, dst_type);
    gen_crlf();
}

nnc_static void nnc_gen_mov_imm_to_reg(const nnc_loc* reg, const nnc_loc* imm) {
    assert(reg->where == L_REG);
    assert(imm->where == L_IMM);
    gen_note("--- mov imm to reg:\n");
    nnc_gen_clear_reg(reg);
    gen_instr(I_MOV);
    nnc_gen_loc(reg, reg->type);
    gen_comma();
    nnc_gen_loc(imm, reg->type);
    gen_crlf();
}

nnc_static void nnc_gen_mov_mem_to_mem(const nnc_loc* dst_mem, const nnc_loc* src_mem) {
    assert(src_mem->where & LOCATION_MEM);
    assert(dst_mem->where & LOCATION_MEM);
    gen_note("--- mov memory to memory:\n");
    assert(false);
}

nnc_static void nnc_gen_mov_reg_to_mem(const nnc_loc* mem, const nnc_loc* reg) {
    assert(reg->where == L_REG);
    assert(mem->where & LOCATION_MEM);
    gen_note("--- mov reg to memory:\n");
    gen_instr(I_MOV);
    nnc_gen_loc(mem, mem->type);
    gen_comma();
    nnc_gen_loc(reg, mem->type);
}

nnc_static void nnc_gen_mov_operand_to_reg(const nnc_loc* reg, const nnc_3a_addr* src) {
    assert(reg->where == L_REG);
    nnc_loc src_loc = nnc_store(src, false);
    gen_note("--- mov operand to reg:\n");
    switch (src_loc.where) {
        case L_IMM:         nnc_gen_mov_imm_to_reg(reg, &src_loc); break;
        case L_REG:         nnc_gen_mov_reg_to_reg(reg, &src_loc); break;
        case L_DATA:        nnc_gen_mov_mem_to_reg(reg, &src_loc); break;
        case L_LOCAL_STACK: nnc_gen_mov_mem_to_reg(reg, &src_loc); break;
        case L_PARAM_STACK: nnc_gen_mov_mem_to_reg(reg, &src_loc); break;
        default: {
            assert(false);
        }
    }
}

nnc_static void nnc_gen_mov_reg_to_operand(const nnc_loc* reg, const nnc_3a_addr* dst) {
    assert(reg->where == L_REG);
    nnc_loc dst_loc = nnc_store(dst, false);
    gen_note("--- mov reg to operand:\n");
    switch (dst_loc.where) {
        case L_REG:         nnc_gen_mov_reg_to_reg(&dst_loc, reg); break;
        case L_DATA:        nnc_gen_mov_reg_to_mem(&dst_loc, reg); break;
        case L_LOCAL_STACK: nnc_gen_mov_reg_to_mem(&dst_loc, reg); break;
        case L_PARAM_STACK: nnc_gen_mov_reg_to_mem(&dst_loc, reg); break;
        default: {
            assert(false);
        }
    }
}

nnc_static void nnc_gen_mov_operand_to_mem(const nnc_loc* mem, const nnc_3a_addr* src) {
    assert(mem->where & LOCATION_MEM);
    nnc_loc src_loc = nnc_store(src, false);
    gen_note("--- mov operand to memory:\n");
    switch (src_loc.where) {
        case L_DATA:
        case L_PARAM_STACK:
        case L_LOCAL_STACK: {
            nnc_gen_mov_mem_to_mem(mem, &src_loc);
            break;
        }
        case L_REG: {
            nnc_gen_mov_reg_to_mem(mem, &src_loc);
            break;
        }
        default: {
            assert(false);
        }
    }
}

nnc_static void nnc_gen_mov_operand_to_operand(const nnc_3a_addr* dst, const nnc_3a_addr* src) {
    assert(!nnc_real_type(dst->type));
    assert(!nnc_struct_or_union_type(dst->type));
    nnc_loc dst_loc = nnc_store(dst, false);
    if (glob_current_quad->op == OP_DEREF_COPY) {
        gen_note("--- mov operand to (deref) operand:\n");
        dst_loc.dereference = true;
        dst_loc.type = src->type;
    }
    else {
        gen_note("--- mov operand to operand:\n");
    }
    switch (dst_loc.where) {
        case L_DATA:
        case L_PARAM_STACK:
        case L_LOCAL_STACK: {
            nnc_gen_mov_operand_to_mem(&dst_loc, src);
            break;
        }
        case L_REG: {
            nnc_gen_mov_operand_to_reg(&dst_loc, src);
            break;
        }
        default: {
            assert(false);
        }
    }
}

nnc_static nnc_bool nnc_cmp_addr(const nnc_3a_addr* addr1, const nnc_3a_addr* addr2) {
    if (addr1->kind != addr2->kind) {
        return false;
    }
    const union _nnc_3a_addr_exact* exact1 = &addr1->exact;
    const union _nnc_3a_addr_exact* exact2 = &addr2->exact;
    switch (addr1->kind) {
        case ADDR_CGT:  return exact1->cgt == exact2->cgt;
        case ADDR_NAME: return nnc_strcmp(
            exact1->name.name, exact2->name.name) == 0;
        // these values are place inside union,
        // so binary representation will be the same
        case ADDR_FCONST:
        case ADDR_ICONST: return exact1->iconst.iconst == 
                                 exact2->iconst.iconst;
        default: {
            nnc_abort_no_ctx("nnc_cmp_addr: unknown addr kind\n");
        }
    }
    return false;
}

nnc_static void nnc_gen_op_deref(const nnc_3a_quad* quad) {
    assert(!nnc_struct_or_union_type(quad->res.type));
    nnc_bool rbx_pushed = false;
    nnc_loc rbx_loc = nnc_mkloc_reg(R_RBX, &u64_type);
    if (!nnc_addr_inside_reg(&quad->arg1, R_RBX)) {
        rbx_pushed = nnc_gen_reserve_reg(R_RBX);
        nnc_gen_mov_operand_to_reg(&rbx_loc, &quad->arg1);
    }
    gen_note("--- deref:\n");
    rbx_loc.dereference = true;
    rbx_loc.type = quad->res.type;
    nnc_gen_mov_reg_to_operand(&rbx_loc, &quad->res);
    nnc_gen_pop_reg(R_RBX, rbx_pushed);
}

nnc_static void nnc_gen_op_copy(const nnc_3a_quad* quad) {
    assert(!nnc_struct_or_union_type(quad->res.type));
    nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
}

nnc_static void nnc_gen_label_decl(const nnc_3a_quad* quad) {
    assert(quad->label != 0);
    gen_t0(NNC_LABEL_PREFIX"%u:", quad->label);
}

nnc_static void nnc_gen_op_ref(const nnc_3a_quad* quad) {
    //todo: fix the case when lea has r/r operands
    nnc_loc dst_loc = nnc_store(&quad->res, false);
    nnc_gen_clear_reg(&dst_loc);
    gen_instr(I_LEA);
    nnc_gen_loc(&dst_loc, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg1, quad->res.type);
}

nnc_static void nnc_gen_op_deref_copy(const nnc_3a_quad* quad) {
    nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
}

nnc_static void nnc_gen_op_unary(const nnc_3a_quad* quad, nnc_asm_instr instr) {
    gen_instr(instr);
    //todo: come back here when optimizations will be presented
    assert(
        quad->arg1.kind == ADDR_CGT
    );
    nnc_gen_operand(&quad->arg1, quad->res.type);
    if (!nnc_cmp_addr(&quad->res, &quad->arg1)) {
        nnc_store(&quad->res, false);
        nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
    }
}

nnc_static void nnc_gen_op_plus(const nnc_3a_quad* quad) {
    //gen_note("OP_PLUS here");
    gen_instr(I_NOP);
}

// https://www.felixcloutier.com/x86/neg
nnc_static void nnc_gen_op_minus(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->res.type));
    nnc_gen_op_unary(quad, I_NEG);
}

// https://www.felixcloutier.com/x86/not
nnc_static void nnc_gen_op_not(const nnc_3a_quad* quad) {
    nnc_gen_op_unary(quad, I_NOT);
}

nnc_static void nnc_gen_op_binary(const nnc_3a_quad* quad, nnc_asm_instr instr) {
    gen_instr(instr);
    //todo: come back here when optimizations will be presented
    assert(
        quad->arg1.kind == ADDR_CGT &&
        (
            quad->arg2.kind == ADDR_CGT ||
            quad->arg2.kind == ADDR_ICONST
        )
    );
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg2, quad->res.type);
    gen_crlf();
    // above code generates binary expressions in format: 
    //     arg1(res) = arg1 binOp arg2, if res == arg1
    // if res != arg1, mov instruction must be generated.
    if (!nnc_cmp_addr(&quad->res, &quad->arg1)) {
        nnc_store(&quad->res, false);
        nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
    }
}

// https://www.felixcloutier.com/x86/add
// https://www.felixcloutier.com/x86/addss
// https://www.felixcloutier.com/x86/addsd
nnc_static void nnc_gen_op_add(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_op_binary(quad, I_ADD);
}

// https://www.felixcloutier.com/x86/sub
// https://www.felixcloutier.com/x86/subss
// https://www.felixcloutier.com/x86/subsd
nnc_static void nnc_gen_op_sub(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_op_binary(quad, I_SUB);
}

/**
 * @brief Generates one operand multiplication instruction.
 *  This is default mode for `mul` instruction.
 *  See more at: https://www.felixcloutier.com/x86/imul#description
 * @param quad 
 * @param mul 
 * @param res_in_rdx When remainder is needed as a result, set it to true.
 * @return 
 */
nnc_static void nnc_gen_one_op_mul(const nnc_3a_quad* quad, nnc_asm_instr mul) {
    nnc_bool rax_pushed = false;
    nnc_bool rdx_pushed = nnc_gen_reserve_reg(R_RDX);
    nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->arg1.type);
    nnc_u64 size = nnc_sizeof(quad->res.type);
    if (!nnc_addr_inside_reg(&quad->arg1, R_RAX)) {
        rax_pushed = nnc_gen_reserve_reg(R_RAX);
        nnc_gen_mov_operand_to_reg(&rax_loc, &quad->arg1);
    }
    gen_instr(mul);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_crlf();
    nnc_pollute(R_RDX, size);
    if (!nnc_addr_inside_reg(&quad->res, R_RAX)) {
        nnc_gen_mov_reg_to_operand(&rax_loc, &quad->res);
    }
    if (!nnc_addr_inside_reg(&quad->arg1, R_RAX)) {
        nnc_gen_pop_reg(R_RAX, rax_pushed);
    }
    nnc_gen_pop_reg(R_RDX, rdx_pushed);
}

/**
 * @brief Generates two operand multiplication instruction.
 *  See more at: https://www.felixcloutier.com/x86/imul#description
 * @param quad 
 * @param mul 
 * @return 
 */
nnc_static void nnc_gen_two_op_mul(const nnc_3a_quad* quad, nnc_asm_instr mul) {
    nnc_gen_op_binary(quad, mul);
}

/**
 * @brief Generates three operand multiplication instruction.
 *  See more at: https://www.felixcloutier.com/x86/imul#description
 * @param quad 
 * @param mul 
 * @return 
 */
nnc_static void nnc_gen_three_op_mul(const nnc_3a_quad* quad, nnc_asm_instr mul) {
    gen_instr(mul);
    nnc_gen_operand(&quad->res, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_comma();
    assert(quad->arg2.kind == ADDR_ICONST);
    nnc_gen_operand(&quad->arg2, quad->arg2.type);
}

/**
 * @brief Checks if multiplication is in two operand form.
 * @param quad 
 * @return 
 */
nnc_static nnc_bool nnc_is_two_op_mul(const nnc_3a_quad* quad) {
    const nnc_loc* arg1_loc = nnc_get_loc(&quad->arg1);
    const nnc_loc* arg2_loc = nnc_get_loc(&quad->arg2);
    if (arg1_loc->where == L_REG) {
        if (arg2_loc->where == L_REG         ||
            arg2_loc->where == L_DATA        ||
            arg2_loc->where == L_LOCAL_STACK ||
            arg2_loc->where == L_PARAM_STACK) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Checks if multiplication is in three operand form.
 * @param quad 
 * @return 
 */
nnc_static nnc_bool nnc_is_three_op_mul(const nnc_3a_quad* quad) {
    const nnc_loc* arg1_loc = nnc_get_loc(&quad->arg1);
    if (arg1_loc->where == L_REG) {
        //todo: check that const is not IMM64
        if (quad->arg2.kind == ADDR_ICONST) {
            return true;
        }
    }
    return false;
} 

// https://www.felixcloutier.com/x86/mul
nnc_static void nnc_gen_mul(const nnc_3a_quad* quad) {
    nnc_gen_one_op_mul(quad, I_MUL);
}

// https://www.felixcloutier.com/x86/imul
nnc_static void nnc_gen_imul(const nnc_3a_quad* quad) {
    if (nnc_is_three_op_mul(quad)) {
        nnc_gen_three_op_mul(quad, I_IMUL);
    }
    else if (nnc_is_two_op_mul(quad)) {
        nnc_gen_two_op_mul(quad, I_IMUL);
    }
    else {
        nnc_gen_one_op_mul(quad, I_IMUL);
    }
}

nnc_static void nnc_gen_op_mul(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->res.type));
    nnc_asm_instr i_mul = I_MUL;
    if (nnc_signed_type(quad->res.type)) {
        i_mul = I_IMUL;
    }
    switch (i_mul) {
        case I_MUL:  nnc_gen_mul(quad);  break;
        case I_IMUL: nnc_gen_imul(quad); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op_mul: unsupported mul instruction.\n");
        }
    }
}

nnc_static void nnc_gen_rdx_extend(const nnc_3a_quad* quad) {
    switch (nnc_sizeof(quad->res.type)) {
        case S_WORD:  gen_t1("cwd\n"); break;
        case S_DWORD: gen_t1("cdq\n"); break;
        case S_QWORD: gen_t1("cqo\n"); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_instr_idiv: bad size of res.");
        }
    }
}

nnc_static void nnc_gen_one_op_div(const nnc_3a_quad* quad, nnc_3a_op_kind div, nnc_bool mod) {
    assert(!nnc_real_type(quad->res.type));
    nnc_bool rax_pushed = false;
    nnc_bool rdx_pushed = nnc_gen_reserve_reg(R_RDX);
    nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->arg1.type);
    nnc_loc rdx_loc = nnc_mkloc_reg(R_RDX, quad->arg1.type);
    nnc_u64 size = nnc_sizeof(quad->arg1.type);
    if (!nnc_addr_inside_reg(&quad->arg1, R_RAX)) {
        rax_pushed = nnc_gen_reserve_reg(R_RAX);
        nnc_gen_mov_operand_to_reg(&rax_loc, &quad->arg1);
    }
    nnc_gen_clear_reg(&rdx_loc);
    if (nnc_signed_type(quad->res.type)) {
        nnc_gen_rdx_extend(quad);
    }
    gen_instr(div);
    nnc_gen_operand(&quad->arg2, quad->res.type);
    gen_crlf();
    nnc_pollute(R_RDX, size);
    if (!mod && !nnc_addr_inside_reg(&quad->res, R_RAX)) {
        nnc_gen_mov_reg_to_operand(&rax_loc, &quad->res);
    }
    if (mod && !nnc_addr_inside_reg(&quad->res, R_RDX)) {
        nnc_gen_mov_reg_to_operand(&rdx_loc, &quad->res);
    }
    if (!nnc_addr_inside_reg(&quad->arg1, R_RAX)) {
        nnc_gen_pop_reg(R_RAX, rax_pushed);
    }
    nnc_gen_pop_reg(R_RDX, rdx_pushed);
}

// https://www.felixcloutier.com/x86/div
nnc_static void nnc_gen_div(const nnc_3a_quad* quad, nnc_bool mod) {
    nnc_gen_one_op_div(quad, I_DIV, mod);
}

// https://www.felixcloutier.com/x86/idiv
nnc_static void nnc_gen_idiv(const nnc_3a_quad* quad, nnc_bool mod) {
    nnc_gen_one_op_div(quad, I_IDIV, mod);
}

nnc_static void nnc_gen_op_div(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_asm_instr i_div = I_DIV;
    if (nnc_signed_type(quad->res.type)) {
        i_div = I_IDIV;
    }
    switch (i_div) {
        case I_DIV:  nnc_gen_div(quad, false);  break;
        case I_IDIV: nnc_gen_idiv(quad, false); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op_div: unsupported div instruction.\n");
        }
    }
}

nnc_static void nnc_gen_op_mod(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_asm_instr i_div = I_DIV;
    if (nnc_signed_type(quad->res.type)) {
        i_div = I_IDIV;
    }
    switch (i_div) {
        case I_DIV:  nnc_gen_div(quad, true);  break;
        case I_IDIV: nnc_gen_idiv(quad, true); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op_mod: unsupported div instruction.\n");
        }
    }
}

nnc_static void nnc_gen_instr_shift(const nnc_3a_quad* quad, nnc_asm_instr shift) {
    nnc_bool rcx_pushed = false;
    nnc_bool arg1_inside_rcx = nnc_addr_inside_reg(&quad->arg1, R_RCX);
    nnc_bool arg2_inside_rcx = nnc_addr_inside_reg(&quad->arg2, R_RCX);
    //todo: support other cases
    assert(
        (!arg1_inside_rcx && arg2_inside_rcx) ||
        (!arg1_inside_rcx && !arg2_inside_rcx)
    );
    assert(quad->arg2.kind != ADDR_ICONST);
    // if second argument is inside RCX
    if (!arg1_inside_rcx && arg2_inside_rcx) {
    }
    // if both arguments are not inside RCX 
    if (!arg1_inside_rcx && !arg2_inside_rcx) {
        // reserve RCX, if it's currently used
        rcx_pushed = nnc_gen_reserve_reg(R_RCX);
        // copy second argument (count of shifts) to RCX
        nnc_loc rcx_loc = nnc_mkloc_reg(R_RCX, quad->arg2.type);
        nnc_gen_mov_operand_to_reg(&rcx_loc, &quad->arg2);
    }
    // generate shift instruction
    gen_instr(shift);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_t0(", cl");
    // restore RCX if it was pushed
    nnc_gen_pop_reg(R_RCX, rcx_pushed);
}

nnc_static void nnc_gen_op_shr(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_instr_shift(quad, I_SHR);
}

nnc_static void nnc_gen_op_shl(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_instr_shift(quad, I_SHL);
}

nnc_static void nnc_gen_op_sal(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_instr_shift(quad, I_SAL);
}

nnc_static void nnc_gen_op_sar(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_instr_shift(quad, I_SAR);
}

// https://www.felixcloutier.com/x86/or
nnc_static void nnc_gen_op_or(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_op_binary(quad, I_OR);
}

// https://www.felixcloutier.com/x86/and
nnc_static void nnc_gen_op_and(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_op_binary(quad, I_AND);
}

// https://www.felixcloutier.com/x86/xor
nnc_static void nnc_gen_op_xor(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_op_binary(quad, I_XOR);
}

nnc_static void nnc_gen_op_arg(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->arg1.type));
    assert(!nnc_struct_or_union_type(quad->arg1.type));
    assert(quad->arg1.kind == ADDR_CGT);
    nnc_loc src = *nnc_get_loc(&quad->arg1);
    nnc_loc dst = nnc_store_arg(&quad->arg1);
    assert(
        dst.where == L_REG || 
        dst.where == L_PARAM_STACK
    );
    gen_instr(I_MOV);
    //todo: apply nnc_gen_mov_operand_to_reg
    if (dst.where == L_REG) {
        nnc_gen_loc(&dst, quad->arg1.type);
    }
    if (dst.where == L_PARAM_STACK) {
        gen_t0("%s ptr [rsp-%lu]", nnc_get_ptr_size(quad->arg1.type), dst.offset);
    }
    gen_comma();
    nnc_gen_loc(&src, quad->arg1.type);
    nnc_call_stack_state_next(&quad->arg1);
}

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retp(const nnc_3a_quad* quad) {
    gen_instr(I_JMP);
    gen_t1(NNC_RET_LABEL_PREFIX"%s", glob_current_proc->name);
}

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retf(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->arg1.type));
    assert(quad->arg1.kind == ADDR_CGT);
    nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->res.type);
    nnc_gen_mov_operand_to_reg(&rax_loc, &quad->arg1);
    gen_instr(I_JMP);
    gen_t0(NNC_RET_LABEL_PREFIX"%s", glob_current_proc->name);
}

nnc_static void nnc_gen_caller_prepare() {
    nnc_gen_alloc_stack(glob_current_call_state->offset);
}

nnc_static void nnc_gen_caller_restore() {
    nnc_gen_dealloc_stack(glob_current_call_state->offset);
    vector(nnc_register) saved_regs = glob_current_call_state->pushed;
    for (nnc_i64 i = 0; i < buf_len(saved_regs); i++) {
        nnc_gen_unreserve_reg(saved_regs[i]);
    }
    if (glob_current_call_state->rax_pushed) {
        nnc_gen_unreserve_reg(R_RAX);
    }
    nnc_call_stack_state_fini();
}

nnc_static void nnc_gen_call(const nnc_3a_addr* call) {
    gen_instr(I_CALL);
    if (call->kind != ADDR_NAME) {
        nnc_gen_operand(call, &u64_type);
    }
    else {
        const char* fn = call->exact.name.name;
        nnc_sym* sym = nnc_st_get_sym(glob_current_ast->st, fn);
        if (sym == NULL) {
            nnc_gen_operand(call, &u64_type);
        }
        else {
            gen_t0("_%s", fn);
        }
    }
    gen_crlf();
}

nnc_static void nnc_gen_op_pcall(const nnc_3a_quad* quad) {
    nnc_gen_caller_prepare();
    nnc_gen_call(&quad->arg1);
    nnc_gen_caller_restore();
}

nnc_static void nnc_gen_op_fcall(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    nnc_gen_caller_prepare();
    nnc_gen_call(&quad->arg1);
    if (!nnc_addr_inside_reg(&quad->res, R_RAX)) {
        nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->res.type);
        nnc_gen_mov_reg_to_operand(&rax_loc, &quad->res);
    }
    nnc_gen_caller_restore();
}

nnc_static void nnc_gen_op_prepare_for_call(const nnc_3a_quad* quad) {
    // if this condition is true, then preparation is done
    // for function call, and we need to preserve RAX or XMM0 register
    nnc_loc call_res_loc = {0};
    nnc_bool rax_pushed = false;
    nnc_register* call_res_reg_loc = NULL;
    if (quad->res.kind != ADDR_NONE) {
        rax_pushed = nnc_gen_reserve_reg(R_RAX);
        if (rax_pushed) {
            call_res_loc = nnc_store(&quad->res, false);
        }
        else {
            call_res_loc.where = L_REG;
            call_res_loc.reg = R_RAX;
        }
        if (call_res_loc.where == L_REG) {
            call_res_reg_loc = &call_res_loc.reg;
        }
    }
    nnc_call_stack_state_init(call_res_reg_loc);
    glob_current_call_state->rax_pushed = rax_pushed;
    vector(nnc_register) pushed = glob_current_call_state->pushed;
    for (nnc_i64 i = buf_len(pushed); i > 0; i--) {
        const nnc_register reg = pushed[i - 1];
        // if this is preparation for function call,
        // we need to allocate storage for the result of 
        // the function, but this appears before the `nnc_call_stack_state_init`,
        // which will think that location (register) is used before call,
        // and will push it, so unpush register where result is stored then.
        if (call_res_loc.where == L_REG && call_res_loc.reg == reg) {
            nnc_unreserve_reg(reg);
        }
        else {
            gen_t1("push %s\n", glob_reg_str[reg]);
        }
    }
}

nnc_static void nnc_gen_label(const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_ICONST);
    gen_t0(NNC_LABEL_PREFIX"%lu\n", addr->exact.iconst.iconst);
}

nnc_static void nnc_gen_instr_cmp(const nnc_3a_quad* quad) {
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    //assert(
    //    nnc_addr_inside(&quad->arg1, L_REG) &&
    //    nnc_addr_inside(&quad->arg2, L_REG | LOCATION_MEM)
    //);
    gen_instr(I_CMP);
    nnc_gen_operand(&quad->arg1, quad->arg1.type);
    gen_comma();
    nnc_gen_operand(&quad->arg2, quad->arg1.type);
    gen_crlf();
}

nnc_static void nnc_gen_instr_cmp_x(const nnc_3a_quad* quad, nnc_u32 with_x) {
    assert(
        quad->arg1.kind == ADDR_CGT || 
        nnc_addr_inside(&quad->arg1, L_REG | LOCATION_MEM)
    );
    gen_instr(I_CMP);
    if (nnc_addr_inside(&quad->arg1, L_REG)) {
        nnc_gen_operand(&quad->arg1, quad->arg1.type);
    }
    else {
        nnc_gen_operand(&quad->arg1, quad->arg1.type);
    }
    gen_t0(", %u", with_x);
    gen_crlf();
}

nnc_static void nnc_gen_instr_jump(const nnc_3a_quad* quad, nnc_asm_instr jump) {
    gen_instr(jump);
    nnc_gen_label(&quad->res);
}

nnc_static void nnc_gen_op_ujump(const nnc_3a_quad* quad) {
    nnc_gen_instr_jump(quad, I_JMP);
}

nnc_static void nnc_gen_op_cjumpt(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp_x(quad, 0);
    nnc_gen_instr_jump(quad, I_JNZ);
}

nnc_static void nnc_gen_op_cjumpf(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp_x(quad, 0);
    nnc_gen_instr_jump(quad, I_JZ);
}

nnc_static void nnc_gen_op_cjumplt(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp(quad);
    nnc_gen_instr_jump(quad, nnc_signed_type(quad->res.type) ? I_JL : I_JB);
}

nnc_static void nnc_gen_op_cjumpgt(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp(quad);
    nnc_gen_instr_jump(quad, nnc_signed_type(quad->res.type) ? I_JG : I_JA);
}

nnc_static void nnc_gen_op_cjumplte(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp(quad);
    nnc_gen_instr_jump(quad, nnc_signed_type(quad->res.type) ? I_JLE : I_JBE);
}

nnc_static void nnc_gen_op_cjumpgte(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp(quad);
    nnc_gen_instr_jump(quad, nnc_signed_type(quad->res.type) ? I_JGE : I_JAE);
}

nnc_static void nnc_gen_op_cjumpe(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp(quad);
    nnc_gen_instr_jump(quad, I_JE);
}

nnc_static void nnc_gen_op_cjumpne(const nnc_3a_quad* quad) {
    nnc_gen_instr_cmp(quad);
    nnc_gen_instr_jump(quad, I_JNE);
}

nnc_static void nnc_gen_memory_init(const char* mem, const char* name, nnc_u64 size) {
    data_put("   %s:", name);
    for (nnc_u64 i = 0, b = 0; i < size; i++, b++) {
        if (b % 8 == 0) {
            data_put("\n.byte ");
            b = 0;
        }
        data_put("0x%02x", (uint8_t)mem[i]);
        if (b != 7 && i != size - 1) {
            data_put(", ");
        }
    }
    data_put("\n");
}

nnc_static void nnc_gen_memory_zero(const char* name, nnc_u64 size) {
    data_put("   %s:", name);
    for (nnc_u64 i = 0, b = 0; i < size; i++, b++) {
        if (b % 8 == 0) {
            data_put("\n.byte ");
            b = 0;
        }
        data_put("0x00");
        if (b != 7 && i != size - 1) {
            data_put(", ");
        }
    }
    data_put("\n");
}

nnc_static void nnc_gen_op_decl_local(const nnc_3a_quad* quad) {
    nnc_store(&quad->res, false);
}

nnc_static void nnc_gen_op_decl_global(const nnc_3a_quad* quad) {
    assert(nnc_get_loc(&quad->res) == NULL);
    nnc_loc loc = nnc_store(&quad->res, true);
    assert(loc.where == L_DATA);
    const nnc_expression* e = (nnc_expression*)quad->hint;
    if (e == NULL) {
        nnc_gen_memory_zero(loc.ds_name, nnc_sizeof(quad->res.type));
    }
    else {
        //todo: make expr to memory conversion
        nnc_i64 value = nnc_evald(e, NULL);
        nnc_gen_memory_init((char*)(&value), loc.ds_name, nnc_sizeof(quad->res.type));
    }
}

nnc_static void nnc_gen_op_decl_string(const nnc_3a_quad* quad) {
    if (nnc_get_loc(&quad->res) != NULL) {
        return;
    }
    nnc_loc loc = nnc_store(&quad->res, true);
    assert(loc.where == L_DATA);
    const nnc_str_literal* sl = (nnc_str_literal*)quad->hint;
    nnc_gen_memory_init(sl->exact, loc.ds_name, sl->bytes + 1);
}

nnc_static void nnc_gen_quad(const nnc_3a_quad* quad) {
    if (quad->label != 0) {
        nnc_gen_label_decl(quad);
    }
    else {
        switch (quad->op) {
            /* Other operators */
            case OP_CAST:       break;
            case OP_REF:        nnc_gen_op_ref(quad);        break;
            case OP_COPY:       nnc_gen_op_copy(quad);       break;
            case OP_DEREF:      nnc_gen_op_deref(quad);      break;
            case OP_DEREF_COPY: nnc_gen_op_deref_copy(quad); break;
            /* Unary operators */
            case OP_PLUS:   nnc_gen_op_plus(quad);  break;
            case OP_MINUS:  nnc_gen_op_minus(quad); break;
            case OP_BW_NOT: nnc_gen_op_not(quad);   break;
            /* Binary operators */
            case OP_ADD:    nnc_gen_op_add(quad); break;
            case OP_SUB:    nnc_gen_op_sub(quad); break;
            case OP_MUL:    nnc_gen_op_mul(quad); break;
            case OP_DIV:    nnc_gen_op_div(quad); break;
            case OP_MOD:    nnc_gen_op_mod(quad); break;
            case OP_SHR:    nnc_gen_op_shr(quad); break;
            case OP_SHL:    nnc_gen_op_shl(quad); break;
            case OP_SAL:    nnc_gen_op_sal(quad); break;
            case OP_SAR:    nnc_gen_op_sar(quad); break;
            case OP_BW_OR:  nnc_gen_op_or(quad);  break;
            case OP_BW_AND: nnc_gen_op_and(quad); break;
            case OP_BW_XOR: nnc_gen_op_xor(quad); break;
            /* Other operators */
            case OP_ARG:   nnc_gen_op_arg(quad);   break;
            case OP_RETP:  nnc_gen_op_retp(quad);  break;
            case OP_RETF:  nnc_gen_op_retf(quad);  break;
            case OP_PCALL: nnc_gen_op_pcall(quad); break;
            case OP_FCALL: nnc_gen_op_fcall(quad); break;
            /* Conditional operators */
            case OP_UJUMP:    nnc_gen_op_ujump(quad);    break;
            case OP_CJUMPT:   nnc_gen_op_cjumpt(quad);   break;
            case OP_CJUMPF:   nnc_gen_op_cjumpf(quad);   break;
            case OP_CJUMPLT:  nnc_gen_op_cjumplt(quad);  break;
            case OP_CJUMPGT:  nnc_gen_op_cjumpgt(quad);  break;
            case OP_CJUMPLTE: nnc_gen_op_cjumplte(quad); break; 
            case OP_CJUMPGTE: nnc_gen_op_cjumpgte(quad); break;
            case OP_CJUMPE:   nnc_gen_op_cjumpe(quad);   break;
            case OP_CJUMPNE:  nnc_gen_op_cjumpne(quad);  break;
            /* Hint operators */
            case OP_HINT_DECL_LOCAL:   nnc_gen_op_decl_local(quad);  break;
            case OP_HINT_DECL_GLOBAL:  nnc_gen_op_decl_global(quad); break;
            case OP_HINT_DECL_STRING:  nnc_gen_op_decl_string(quad); break;
            case OP_HINT_PREPARE_CALL: nnc_gen_op_prepare_for_call(quad); break;
            default: {
                nnc_abort_no_ctx("nnc_gen_op: unknown operator.\n");
            }
        }
    }
}

nnc_static void nnc_gen_proc_prologue(nnc_3a_proc* proc) {
    if (proc->stack_usage > 0 || proc->local_stack_offset > 0) {
        gen_t1("push rbp\n");
        gen_t1("mov rbp, rsp\n");
    }
    if (proc->local_stack_offset > 0) {
        nnc_gen_alloc_stack(proc->local_stack_offset);
    }
}

nnc_static void nnc_gen_proc_epilogue(nnc_3a_proc* proc) {
    if (proc->stack_usage > 0 || proc->local_stack_offset > 0) {
        gen_t1("leave\n");
    }
}

nnc_static nnc_u32 nnc_callee_calc_params_stack() {
    nnc_u32 stack_usage = 0;
    const vector(nnc_fn_param*) params = glob_current_proc->params;
    nnc_3a_addr param = { .kind = ADDR_NAME };
    nnc_call_stack_state_init(NULL);
    assert(glob_current_call_state->pushed == NULL);
    for (nnc_u64 i = 0; i < buf_len(params); i++) {
        param.type = params[i]->var->type;
        param.exact.name.name = params[i]->var->name;
        nnc_loc loc = nnc_store_param(&param);
        if (loc.where == L_PARAM_STACK) {
            stack_usage += param.type->size;
        }
        nnc_call_stack_state_next(&param);
    }
    nnc_call_stack_state_fini();
    return stack_usage;
}

nnc_static void nnc_gen_callee_spill_params(nnc_3a_proc* proc) {
    // parameters stored in regsiters according to x86_64 ABI
    // are spilled to memory to avoid them shuffled or erased 
    // during their use in other function called inside.
    gen_note("--- spill params start\n");
    nnc_3a_addr param = { .kind = ADDR_NAME };
    const vector(nnc_fn_param*) params = proc->params;
    for (nnc_u64 i = 0; i < buf_len(params); i++) {
        param.type = params[i]->var->type;
        param.exact.name.name = params[i]->var->name;
        const nnc_loc* old = nnc_get_loc(&param);
        if (old == NULL) {
            continue;
        }
        nnc_loc reg = *old;
        // discards `const`, bad approach
        nnc_loc mem = nnc_spill_param(&param);
        assert(mem.where & LOCATION_MEM);
        nnc_gen_reg_to_mem_mov(&reg, &mem);
    }
    gen_note("--- spill params end\n");
}

nnc_static void nnc_callee_store_params(nnc_3a_proc* proc) {
    glob_current_proc->stack_usage = nnc_callee_calc_params_stack();
    const vector(nnc_fn_param*) params = glob_current_proc->params;
    nnc_3a_addr param = { .kind = ADDR_NAME };
    nnc_call_stack_state_init(NULL);
    for (nnc_u64 i = 0; i < buf_len(params); i++) {
        param.type = params[i]->var->type;
        param.exact.name.name = params[i]->var->name;
        nnc_store_param(&param);
        nnc_call_stack_state_next(&param);
    }
    nnc_call_stack_state_fini();
}

nnc_static void nnc_gen_callee_prepare() {
    nnc_gen_proc_prologue(glob_current_proc);
}

nnc_static void nnc_gen_callee_restore() {
    gen_t0(NNC_RET_LABEL_PREFIX"%s:\n", glob_current_proc->name);
    nnc_gen_proc_epilogue(glob_current_proc);
    gen_t1("ret\n\n");
}

extern vector(nnc_3a_lr*) glob_reg_lr[21];
extern dictionary(const char*, nnc_loc*) glob_loc_map;
extern void nnc_dump_3a_quad(const nnc_3a_quad* quad);

nnc_static void nnc_callee_clear_lr() {
    // clear register live ranges from previous procudure
    for (nnc_u64 i = 0; i < nnc_arr_size(glob_reg_lr); i++) {
        buf_free(glob_reg_lr[i]);
    }
    memset(glob_reg_lr, 0, sizeof(glob_reg_lr));
}

nnc_static void nnc_callee_clear_junk() {
    memset(glob_reg_junk_level, REG_HAS_JUNK_64, sizeof(glob_reg_junk_level));
}

nnc_static void nnc_reloc_global_symbol(nnc_map_key key, nnc_map_val val) {
    const char* label = (char*)key;
    const nnc_loc* loc = (nnc_loc*)val;
    if (loc->where != L_DATA) {
        nnc_dispose((nnc_heap_ptr)key);
        nnc_dispose((nnc_heap_ptr)val);
    }
    else {
        map_put_s(glob_loc_map, label, loc);
    }
}

nnc_static void nnc_callee_clear_loc_map() {
    if (glob_loc_map == NULL) {
        return;
    }
    nnc_map* prev_loc_map = glob_loc_map;
    glob_loc_map = map_init_with(16);
    nnc_map_iter(prev_loc_map, nnc_reloc_global_symbol);
    map_fini(prev_loc_map);
}

#define GEN_DEBUG 1

#if 1 || GEN_DEBUG == 0
    #undef MAG
    #undef RED
    #undef GRN
    #undef RESET
    #define MAG ""
    #define RED ""
    #define GRN ""
    #define RESET "" 
#endif

nnc_static nnc_assembly_proc nnc_build_current_proc(const nnc_blob_buf* code_impl) {
    #if GEN_DEBUG == 1
        gen_t0(MAG);
    #endif
    gen_t0("_%s:\n", glob_current_asm_proc.code->name);
    #if GEN_DEBUG == 1
        gen_t0(RESET);
    #endif
    nnc_gen_callee_prepare();
    nnc_blob_buf_append(&glob_current_asm_proc.impl, code_impl);
    nnc_gen_callee_restore();
    return glob_current_asm_proc;
}

nnc_static nnc_assembly_proc nnc_gen_proc(nnc_3a_proc* proc_3a) {
    glob_current_proc = proc_3a;
    glob_current_asm_proc = (nnc_assembly_proc){
        .code = proc_3a,
        .impl = (nnc_blob_buf){0}
    };
    nnc_blob_buf_init(&glob_current_asm_proc.impl);
    nnc_callee_clear_lr();
    nnc_callee_clear_junk();
    nnc_callee_clear_loc_map();
    // store params
    nnc_callee_store_params(proc_3a);
    //todo: improve condition when this is applied.
    nnc_gen_callee_spill_params(proc_3a);
    for (nnc_u64 i = 0; i < buf_len(proc_3a->quads); i++) {
        const nnc_3a_quad* quad = &proc_3a->quads[i];
        #if GEN_DEBUG == 1
            gen_t1("# ");
            gen_t0(RED);
            nnc_dump_3a_quad(quad);
            gen_t0(GRN);
        #endif
        nnc_gen_quad(quad);
        #if GEN_DEBUG == 1
            gen_t0(RESET);
        #endif
        gen_crlf();
        glob_current_asm_proc.code->quad_pointer++;
    }
    nnc_blob_buf code_impl = glob_current_asm_proc.impl;
    glob_current_asm_proc.impl = (nnc_blob_buf){0};
    nnc_blob_buf_init(&glob_current_asm_proc.impl);
    nnc_build_current_proc(&code_impl);
    nnc_blob_buf_fini(&code_impl);
    return glob_current_asm_proc;
}

nnc_assembly_file nnc_gen(vector(nnc_3a_proc) procs_3a) {
    glob_current_asm_file = (nnc_assembly_file){
        .procs = NULL,
        .entry_here = false,
        .data_segment_impl = (nnc_blob_buf){0}
    };
    nnc_blob_buf_init(&glob_current_asm_file.data_segment_impl);
    for (nnc_u64 i = 0; i < buf_len(procs_3a); i++) {
        nnc_assembly_proc proc = nnc_gen_proc(&procs_3a[i]);
        if (nnc_strcmp(proc.code->name, "main")) {
            glob_current_asm_file.entry_here = true;
        }
        buf_add(glob_current_asm_file.procs, proc);
    }
    return glob_current_asm_file;
}

nnc_blob_buf nnc_build(nnc_assembly_file file) {
    nnc_blob_buf impl = (nnc_blob_buf){0};
    nnc_blob_buf_init(&impl);
    nnc_blob_buf_putf(&impl,
        ".intel_syntax noprefix \n"
        "\n"
        ".global _start\n"
        ".text\n"
        "\n"
        "_write:\n"
        "   mov rax, 1\n"
        "   syscall\n"
        "\n"
        "_exit:\n"
        "   mov rax, 60\n"
        "   syscall\n"
        "\n"
        "_start:\n"
        "   pop rdi\n"
        "   mov rsi, rsp\n"
        "   call _main\n"
        "   mov rdi, rax\n"
        "   call _exit\n"
        "   hlt\n\n"
    );
    for (nnc_u64 i = 0; i < buf_len(file.procs); i++) {
        nnc_blob_buf_append(&impl, &file.procs[i].impl);
        nnc_blob_buf_fini(&file.procs[i].impl);
    }
    if (file.data_segment_impl.len != 0) {
        nnc_blob_buf_putf(&impl, ".data\n");
        nnc_blob_buf_append(&impl, &file.data_segment_impl);
        nnc_blob_buf_fini(&file.data_segment_impl);
    }
    return impl;
}