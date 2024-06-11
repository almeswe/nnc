#include "nnc_gen.h"
#include "nnc_state.h"

//todo: cfi or .loc directives??
// https://sourceware.org/binutils/docs/as/
// https://docs.oracle.com/cd/E53394_01/html/E54851/eoiyg.html

#define nnc_mkloc_reg(r, t) (nnc_loc){ .where = L_REG, .exact.reg = r, .type = t }

#define REG_LOC(r) (nnc_loc) { .where = L_REG, .exact.reg = r }

#define REG_HAS_JUNK_0  (0)
#define REG_HAS_JUNK_8  (1)
#define REG_HAS_JUNK_16 (2)
#define REG_HAS_JUNK_32 (4)
#define REG_HAS_JUNK_64 (8)

#define NNC_STACK_ALIGN 16

#define NNC_RT_PATH   "nnc_rt.o"
#define NNC_CRT1_PATH "crt1.o"
#define NNC_CRTi_PATH "crti.o"
#define NNC_CRTn_PATH "crtn.o"

#define NNC_MEM_PAD_SIZE 8

#define glob_quad glob_asm_proc->quads[glob_asm_proc->quad_pointer]

#define GEN_INST(x)   text_put("  %s ", x)
#define GEN_TEXT(...) text_put(__VA_ARGS__)
#define GEN_DATA(...) data_put(__VA_ARGS__)
#if 0
#define GEN_NOTE(...) text_put("# " __VA_ARGS__)
#else
#define GEN_NOTE(...)
#endif

nnc_static nnc_u8 glob_junk_vec[21] = {0};
nnc_static vector(nnc_pclass_state) glob_call_states = NULL;

nnc_static void nnc_reset_junk_vec() {
    memset(glob_junk_vec, REG_HAS_JUNK_64, sizeof(glob_junk_vec));
}

nnc_static void nnc_gen_push(const nnc_loc* loc) {
    assert(loc->where == L_REG);
    GEN_INST(I_PUSH);
    GEN_TEXT(glob_reg_str[loc->exact.reg]);
    GEN_TEXT("\r\n");
    glob_asm_proc->stack_alloc += 8;
}

nnc_static void nnc_gen_pop(const nnc_loc* loc) {
    assert(loc->where == L_REG);
    GEN_INST(I_POP);
    GEN_TEXT(glob_reg_str[loc->exact.reg]);
    GEN_TEXT("\r\n");
    glob_asm_proc->stack_alloc -= 8;
    assert(glob_asm_proc->stack_alloc >= 0);
}

nnc_static void nnc_gen_alloc_stack(nnc_u32 bytes) {
    if (bytes != 0) {
        GEN_INST(I_SUB);
        GEN_TEXT("rsp,%u\n", bytes);
        glob_asm_proc->stack_alloc += bytes;
    }
}

nnc_static void nnc_gen_dealloc_stack(nnc_u32 bytes) {
    if (bytes != 0) {
        GEN_INST(I_ADD);
        GEN_TEXT("rsp,%u\n", bytes);
        glob_asm_proc->stack_alloc -= bytes;
    }
    assert(glob_asm_proc->stack_alloc >= 0);
}

nnc_static nnc_u32 nnc_get_stack_pad(nnc_u32 bytes) {
    // calculate amount of stack space to be added
    // for performing 16byte stack alignment required by the ABI
    nnc_u32 pad = bytes % NNC_STACK_ALIGN;
    if (pad != 0) {
        pad = (NNC_STACK_ALIGN - pad) % NNC_STACK_ALIGN;
    }
    return pad;
}

nnc_static nnc_u32 nnc_gen_align_stack(nnc_u32 bytes) {
    nnc_u32 pad = nnc_get_stack_pad(bytes);
    if (pad != 0) {
        GEN_TEXT("# alignment\n");
        if (pad == PS_QWORD) {
            nnc_gen_push(&REG_LOC(R_R15));
        }
        else {
            GEN_INST(I_AND), GEN_TEXT("rsp,0xfffffffffffffff0");
            glob_asm_proc->stack_alloc += nnc_get_stack_pad(glob_asm_proc->stack_alloc);
            GEN_TEXT("\r\n");
        }
    }
    return pad;
}


nnc_static const char* nnc_get_ptr_size(const nnc_type* type) {
    if (nnc_arr_or_ptr_type(type)) {
        return P_QWORD;
    }
    switch (nnc_sizeof(type)) {
        case PS_BYTE:  return P_BYTE;
        case PS_WORD:  return P_WORD;
        case PS_DWORD: return P_DWORD;
        case PS_QWORD: return P_QWORD;
        default: {
            nnc_abort_no_ctx("nnc_get_ptr_size: unsupported size.\n");
        }
    }
    return NULL;
}

nnc_static const char* nnc_get_reg_of_size(nnc_reg reg, nnc_u64 size) {
    const static char* _int[][4] = {
        [R_RAX]  = { "al",   "ax",   "eax",  "rax" },
        [R_RBX]  = { "bl",   "bx",   "ebx",  "rbx" },
        [R_RCX]  = { "cl",   "cx",   "ecx",  "rcx" },
        [R_RDX]  = { "dl",   "dx",   "edx",  "rdx" },
        [R_RSI]  = { "sil",  "si",   "esi",  "rsi" },
        [R_RDI]  = { "dil",  "di",   "edi",  "rdi" },
        [R_R8]   = { "r8b",  "r8w",  "r8d",  "r8"  },
        [R_R9]   = { "r9b",  "r9w",  "r9d",  "r9"  },
        [R_R10]  = { "r10b", "r10w", "r10d", "r10" },
        [R_R11]  = { "r11b", "r11w", "r11d", "r11" },
        [R_R12]  = { "r12b", "r12w", "r12d", "r12" },
        [R_R13]  = { "r13b", "r13w", "r13d", "r13" },
        [R_R14]  = { "r14b", "r14w", "r14d", "r14" },
        [R_R15]  = { "r15b", "r15w", "r15d", "r15" }
    };
    const static char* _sse[] = {
        [R_XMM0 - R_XMM0] = "xmm0",
        [R_XMM1 - R_XMM0] = "xmm1",
        [R_XMM2 - R_XMM0] = "xmm2", 
        [R_XMM3 - R_XMM0] = "xmm3",
        [R_XMM4 - R_XMM0] = "xmm4",
        [R_XMM5 - R_XMM0] = "xmm5",
        [R_XMM6 - R_XMM0] = "xmm6",
        [R_XMM7 - R_XMM0] = "xmm7" 
    };
    if (reg >= R_XMM0 && reg <= R_XMM7) {
        return _sse[reg - R_XMM0];
    }
    else {
        switch (size) {
            case PS_BYTE:  return _int[reg][0];
            case PS_WORD:  return _int[reg][1];
            case PS_DWORD: return _int[reg][2];
            case PS_QWORD: return _int[reg][3];
            default: {
                nnc_abort_no_ctx("nnc_get_reg_of_size: unsupported size.\n");
            }
        }
    }
    return NULL;
}

nnc_static const char* nnc_get_reg_of_type(nnc_reg reg, const nnc_type* type) {
    if (nnc_arr_or_ptr_type(type)) {
        return nnc_get_reg_of_size(reg, PS_QWORD);
    }
    if (nnc_real_type(type)) {
        return nnc_get_reg_of_size(reg, PS_QWORD);
    }
    else {
        return nnc_get_reg_of_size(reg, nnc_sizeof(type));
    }
}

nnc_static nnc_bool nnc_addr_inside_reg(const nnc_3a_addr* addr, nnc_reg reg) {
    const nnc_loc* loc = nnc_get_loc(addr);
    if (loc && loc->where == L_REG) {
        return loc->exact.reg == reg;
    }
    return false;
}

nnc_static nnc_bool nnc_addr_inside(const nnc_3a_addr* addr, nnc_loc_where at) {
    const nnc_loc* loc = nnc_get_loc(addr);
    return loc && (loc->where & at) != 0;
}

nnc_static void nnc_pollute(nnc_reg reg, nnc_u8 amount) {
    assert(
        amount == 0 ||
        amount == 1 ||
        amount == 2 ||
        amount == 4 ||
        amount == 8
    );
    glob_junk_vec[reg] = amount;
}

nnc_static nnc_bool nnc_gen_reserve_reg(nnc_reg reg) {
    nnc_bool need_push = nnc_reserve_reg(reg);
    if (need_push) {
        nnc_gen_push(&REG_LOC(reg));
    }
    return need_push;
}

nnc_static void nnc_gen_pop_reg(nnc_reg reg, nnc_bool pushed) {
    nnc_unreserve_reg(reg);
    if (pushed) {
        nnc_loc loc = {
            .exact.reg = reg,
            .where = L_REG
        };
        nnc_gen_pop(&loc);
        nnc_pollute(reg, REG_HAS_JUNK_64);
    }
}

nnc_static void nnc_gen_unreserve_reg(nnc_reg reg) {
    nnc_gen_pop_reg(reg, true);
}

nnc_static void nnc_gen_loc(const nnc_loc* at, const nnc_type* type) {
    //todo: refactor this
    switch (at->where) {
        case L_REG: {
            if (at->deref) {
                const char* ptr_size = nnc_get_ptr_size(type);
                GEN_TEXT("%s ptr [%s]", ptr_size, glob_reg_str[at->exact.reg]);
            }
            else {
                GEN_TEXT("%s", nnc_get_reg_of_type(at->exact.reg, type));
            }
            break;
        }
        case L_STACK: {
            const char* ptr_size = nnc_get_ptr_size(type);
            if (at->exact.mem <= 0) {
                GEN_TEXT("%s ptr [rbp%d]", ptr_size, at->exact.mem);
            }
            else {
                GEN_TEXT("%s ptr [rbp+%d]", ptr_size, at->exact.mem);
            }
            break;
        }
        case L_DATA: {
            if (at->has_offset) {
                GEN_TEXT("offset ");    
            }
            if (at->deref) {
                const char* ptr_size = nnc_get_ptr_size(type);
                GEN_TEXT("%s ptr [%s]", ptr_size, at->exact.dsl);
            }
            else {
                GEN_TEXT("%s", at->exact.dsl);
            }
            break;
        }
        case L_IMM: {
            GEN_TEXT("0x%lx", at->exact.imm.d);
            break;
        }
        default: {
            nnc_abort_no_ctx("nnc_gen_storage: unsupported storage type.\n");
        }
    }
}

nnc_static void nnc_gen_operand(const nnc_3a_addr* addr, const nnc_type* type) {
    nnc_loc loc = nnc_store(addr);
    nnc_gen_loc(&loc, type);
}

nnc_static void nnc_gen_xor_reg(const nnc_loc* at) {
    assert(at && at->where == L_REG);
    nnc_u64 junk = glob_junk_vec[at->exact.reg];
    if (junk != REG_HAS_JUNK_0) {
        const char* reg_str = nnc_get_reg_of_size(at->exact.reg, junk);
        GEN_INST(I_XOR);
        GEN_TEXT("%s,%s\n", reg_str, reg_str);
    }
    nnc_pollute(at->exact.reg, REG_HAS_JUNK_0);
}

nnc_static void nnc_gen_erase_reg(const nnc_loc* at) {
    assert(at && at->where == L_REG);
    nnc_u64 size = REG_HAS_JUNK_64;
    nnc_u64 junk = glob_junk_vec[at->exact.reg];
    if (!nnc_arr_type(at->type)) {
        size = nnc_sizeof(at->type);
    }
    // make it not generate xor when trashed by 64bits, but write 32bits.
    if (junk != REG_HAS_JUNK_0  &&
        junk == REG_HAS_JUNK_64 && size == PS_DWORD) {
        nnc_pollute(at->exact.reg, REG_HAS_JUNK_32);
    }
    else {
        if (junk > size) {
            GEN_NOTE("---- reg %s has %d junked byte(s)\n", glob_reg_str[at->reg], junk);
            nnc_gen_xor_reg(at);
        }
        nnc_pollute(at->exact.reg, size);
    }
}

nnc_static nnc_bool nnc_gen_int_conv(const nnc_loc* dst_loc, const nnc_loc* src_loc) {
    nnc_asm_inst i_mov = I_MOV;
    nnc_u64 dst_type_size = nnc_sizeof(dst_loc->type);
    nnc_u64 src_type_size = nnc_sizeof(src_loc->type);
    nnc_bool is_u = nnc_unsigned_type(dst_loc->type);
    if (dst_type_size < src_type_size) {
        return false;
    }
    switch (dst_type_size) {
        case PS_BYTE: {
            i_mov = I_MOV;
            break;
        }
        case PS_WORD: {
            switch (src_type_size) {
                case PS_BYTE: i_mov = is_u ? I_MOVZX : I_MOVSX;  break;
                case PS_WORD: i_mov = I_MOV; break;
                default: assert(false);
            }
            break;
        }
        case PS_DWORD: {
            switch (src_type_size) {
                case PS_BYTE:  i_mov = is_u ? I_MOVZX : I_MOVSX; break;
                case PS_WORD:  i_mov = is_u ? I_MOVZX : I_MOVSX; break; 
                case PS_DWORD: i_mov = I_MOV; break;
                default: assert(false);
            }
            break;
        }
        case PS_QWORD: {
            switch (src_type_size) {
                case PS_BYTE:  i_mov = is_u ? I_MOVZX : I_MOVSX;  break;
                case PS_WORD:  i_mov = is_u ? I_MOVZX : I_MOVSX;  break;
                case PS_DWORD: i_mov = is_u ? I_MOV   : I_MOVSXD; break;
                case PS_QWORD: i_mov = I_MOV; break;
                default: assert(false);
            }
            break;
        }
        default: assert(false);
    }
    const nnc_type* dst_type = dst_loc->type;
    const nnc_type* src_type = src_loc->type;
    // special case for unsigned extension of r64 with r/m32
    if (is_u && dst_type_size == PS_QWORD && src_type_size == PS_DWORD) {
        dst_type = src_type;
    }
    if (dst_loc->where == L_REG) {
        nnc_gen_erase_reg(dst_loc);
    }
    GEN_INST(i_mov);
    nnc_gen_loc(dst_loc, dst_type);
    GEN_TEXT(",");
    nnc_gen_loc(src_loc, src_type);
    GEN_TEXT("\r\n");
    return true;
}

nnc_static nnc_bool nnc_gen_conv(const nnc_loc* dst_loc, const nnc_loc* src_loc) {
    GEN_NOTE("--- conversion:\n");
    if (nnc_integral_type(dst_loc->type) && nnc_integral_type(src_loc->type)) {
        return nnc_gen_int_conv(dst_loc, src_loc);
    }
    else {
        return false;
    }
}

nnc_static void nnc_gen_mov_mem_to_reg(const nnc_loc* reg, const nnc_loc* mem) {
    assert(mem->where & L_MEM);
    nnc_loc dst_loc = *reg;
    nnc_loc src_loc = *mem;
    GEN_NOTE("--- mov memory to reg:\n");
    const char* i_mov = I_MOV;
    if (glob_quad.op == OP_CAST) {
        if (nnc_gen_conv(&dst_loc, &src_loc)) {
            return;
        }
    }
    if (nnc_arr_type(src_loc.type)) {
        if (src_loc.where == L_DATA) {
            src_loc.has_offset = true;
        }
        else {
            i_mov = I_LEA;
        }
    }
    nnc_gen_erase_reg(&dst_loc);
    GEN_INST(i_mov);
    nnc_gen_loc(&dst_loc, src_loc.type);
    GEN_TEXT(",");
    nnc_gen_loc(&src_loc, src_loc.type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_reg_to_mem_mov(const nnc_loc* reg, const nnc_loc* mem) {
    assert(reg->where == L_REG);
    assert(mem->where & L_MEM);
    GEN_NOTE("--- mov reg to memory:\n");
    //todo: maybe not regular mov?
    GEN_INST(I_MOV);
    nnc_gen_loc(mem, reg->type);
    GEN_TEXT(",%s\r\n", nnc_get_reg_of_type(reg->exact.reg, reg->type));
}

nnc_static void nnc_gen_mov_reg_to_reg(const nnc_loc* dst_reg, const nnc_loc* src_reg) {
    assert(src_reg->where == L_REG);
    nnc_loc src_loc = *src_reg;
    nnc_loc dst_loc = *dst_reg;
    GEN_NOTE("--- mov reg to reg:\n");
    GEN_NOTE("----- dst reg junked by %d:\n", glob_junk_vec[dst_loc.reg]);
    if (glob_quad.op == OP_CAST) {
        if (nnc_gen_conv(&dst_loc, &src_loc)) {
            return;
        }
    }
    if (!dst_loc.deref) {
        if (src_loc.exact.reg == dst_loc.exact.reg) {
            return;
        }
        nnc_gen_erase_reg(&dst_loc);
    }
    if (src_loc.deref) {
        // if dereferencing pointer to array,
        // just mov, don't need to dereference
        if (nnc_arr_type(dst_loc.type)) {
            src_loc.deref = false;
        }
    }
    GEN_INST(I_MOV);
    nnc_gen_loc(&dst_loc, dst_loc.type);
    GEN_TEXT(",");
    nnc_gen_loc(&src_loc, dst_loc.type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_mov_imm_to_reg(const nnc_loc* reg, const nnc_loc* imm) {
    assert(reg->where == L_REG);
    assert(imm->where == L_IMM);
    GEN_NOTE("--- mov imm to reg:\n");
    nnc_gen_erase_reg(reg);
    GEN_INST(I_MOV);
    nnc_gen_loc(reg, reg->type);
    GEN_TEXT(",");
    nnc_gen_loc(imm, reg->type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_mov_reg_to_mem(const nnc_loc* mem, const nnc_loc* reg) {
    assert(reg->where == L_REG);
    assert(mem->where & L_MEM);
    GEN_NOTE("--- mov reg to memory:\n");
    GEN_INST(I_MOV);
    nnc_gen_loc(mem, mem->type);
    GEN_TEXT(",");
    nnc_gen_loc(reg, mem->type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_mov_mem_to_mem(const nnc_loc* dst_mem, const nnc_loc* src_mem) {
    assert(src_mem->where & L_MEM);
    assert(dst_mem->where & L_MEM);
    GEN_NOTE("--- mov memory to memory:\n");
    nnc_bool pushed = nnc_gen_reserve_reg(R_R15);
    nnc_loc dst_reg = { 
        .where = L_REG,
        .exact.reg = R_R15,
        .type = dst_mem->type
    };
    nnc_gen_mov_mem_to_reg(&dst_reg, src_mem);
    nnc_gen_mov_reg_to_mem(dst_mem, &dst_reg);
    nnc_gen_pop_reg(R_R15, pushed);
}

nnc_static void nnc_gen_mov_imm_to_mem(const nnc_loc* mem, const nnc_loc* imm) {
    assert(imm->where == L_IMM);
    assert(mem->where & L_MEM);
    assert(imm->exact.imm.u <= UINT32_MAX);
    GEN_INST(I_MOV);
    nnc_gen_loc(mem, mem->type);
    GEN_TEXT(",");
    nnc_gen_loc(imm, mem->type);
}

nnc_static void nnc_gen_mov_operand_to_reg(const nnc_loc* reg, const nnc_3a_addr* src) {
    assert(reg->where == L_REG);
    nnc_loc src_loc = nnc_store(src);
    GEN_NOTE("--- mov operand to reg:\n");
    switch (src_loc.where) {
        case L_IMM:   nnc_gen_mov_imm_to_reg(reg, &src_loc); break;
        case L_REG:   nnc_gen_mov_reg_to_reg(reg, &src_loc); break;
        case L_DATA:  nnc_gen_mov_mem_to_reg(reg, &src_loc); break;
        case L_STACK: nnc_gen_mov_mem_to_reg(reg, &src_loc); break;
        default: {
            assert(false);
        }
    }
}

nnc_static void nnc_gen_mov_reg_to_operand(const nnc_loc* reg, const nnc_3a_addr* dst) {
    assert(reg->where == L_REG);
    nnc_loc dst_loc = nnc_store(dst);
    GEN_NOTE("--- mov reg to operand:\n");
    switch (dst_loc.where) {
        case L_REG:   nnc_gen_mov_reg_to_reg(&dst_loc, reg); break;
        case L_DATA:  nnc_gen_mov_reg_to_mem(&dst_loc, reg); break;
        case L_STACK: nnc_gen_mov_reg_to_mem(&dst_loc, reg); break;
        default: {
            assert(false);
        }
    }
}

nnc_static void nnc_gen_mov_operand_to_mem(const nnc_loc* mem, const nnc_3a_addr* src) {
    assert(mem->where & L_MEM);
    nnc_loc src_loc = nnc_store(src);
    nnc_loc dst_loc = *mem;
    GEN_NOTE("--- mov operand to memory:\n");
    if (dst_loc.where == L_DATA) {
        dst_loc.deref = true;
    }
    switch (src_loc.where) {
        case L_REG:   nnc_gen_mov_reg_to_mem(&dst_loc, &src_loc); break;
        case L_IMM:   nnc_gen_mov_imm_to_mem(&dst_loc, &src_loc); break;
        case L_DATA:  nnc_gen_mov_mem_to_mem(&dst_loc, &src_loc); break;
        case L_STACK: nnc_gen_mov_mem_to_mem(&dst_loc, &src_loc); break;
        default: {
            assert(false);
        }
    }
}

nnc_static void nnc_gen_mov_operand_to_operand(const nnc_3a_addr* dst, const nnc_3a_addr* src) {
    assert(!nnc_real_type(dst->type));
    assert(!nnc_struct_or_union_type(dst->type));
    nnc_loc dst_loc = nnc_store(dst);
    if (glob_quad.op == OP_DEREF_COPY) {
        GEN_NOTE("--- mov operand to (deref) operand:\n");
        dst_loc.deref = true;
        dst_loc.type = src->type;
    }
    else {
        GEN_NOTE("--- mov operand to operand:\n");
    }
    switch (dst_loc.where) {
        case L_REG:   nnc_gen_mov_operand_to_reg(&dst_loc, src); break;
        case L_DATA:  nnc_gen_mov_operand_to_mem(&dst_loc, src); break;
        case L_STACK: nnc_gen_mov_operand_to_mem(&dst_loc, src); break;
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
        // these values are placed inside union,
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

/**
 * @brief Generates initialized memory on data segment.
 *  //todo: apply .rept?
 * @param mem Memory for initialization.
 * @param name Name of the memory span.
 * @param size Size of memory span in bytes.
 */
nnc_static void nnc_gen_mem(const char* mem, const char* name, nnc_u64 size) {
    GEN_DATA("   %s:\n", name);
    GEN_DATA("# init'd pad of %lu bytes", size);
    for (nnc_u64 i = 0, b = 0; i < size; i++, b++) {
        if (b % 8 == 0) {
            GEN_DATA("\n.byte ");
            b = 0;
        }
        GEN_DATA("0x%02x", (uint8_t)mem[i]);
        if (b != 7 && i != size - 1) {
            GEN_DATA(", ");
        }
    }
    GEN_DATA("\n");
    GEN_DATA("# init'd pad ends\n");
}

/**
 * @brief Generates zeroed memory pad on data segment.
 * @param size Size of memory pad in bytes.
 */
nnc_static void nnc_gen_mem_pad(nnc_u64 size) {
    GEN_DATA("# zero pad of %lu bytes", size);
    for (nnc_u64 i = 0, b = 0; i < size; i++, b++) {
        if (b % 8 == 0) {
            GEN_DATA("\n.byte ");
            b = 0;
        }
        GEN_DATA("0x00");
        if (b != 7 && i != size - 1) {
            GEN_DATA(", ");
        }
    }
    GEN_DATA("\n");
    GEN_DATA("# zero pad ends\n");
}

/**
 * @brief Generates named memory span on data segment (all zeros).
 * @param name Name of the memory span (label).
 * @param size Size of memory span in bytes.
 */
nnc_static void nnc_gen_mem_named(const char* name, nnc_u64 size) {
    GEN_DATA("   %s:\n", name);
    nnc_gen_mem_pad(size);
}

/**
 * @brief Generates assembly for global variable declaration in data segment.
 * @param var Intermediate representation for variable declaration. 
 */
nnc_static void nnc_gen_var(nnc_ir_var* var) {
    const nnc_loc loc = nnc_store(&var->res);
    assert(nnc_get_loc(&var->res) != NULL); 
    assert(nnc_get_loc(&var->res)->where == L_DATA);
    assert(var->let != NULL);
    assert(loc.where == L_DATA);
    const nnc_expression* expr = var->let->init;
    const nnc_type* type = var->let->texpr->type; 
    const nnc_u64 bytes = nnc_sizeof(type);
    if (expr == NULL) {
        nnc_gen_mem_named(loc.exact.dsl, bytes);
    }
    else {
        //todo: make expr to memory conversion
        nnc_i64 value = nnc_evald(expr, NULL);
        nnc_gen_mem((const char*)(&value), loc.exact.dsl, bytes);
    }
    // if array is declared on data segment, 
    // add 8byte padding after it, so there was less
    // corruption in case of buffer overflow
    if (nnc_arr_type(type)) {
        nnc_gen_mem_pad(NNC_MEM_PAD_SIZE);
    }
}

nnc_static void nnc_gen_label(const nnc_3a_addr* addr) {
    assert(addr->kind == ADDR_ICONST);
    GEN_TEXT(DR_LABEL"%lu\n", addr->exact.iconst.iconst);
}

nnc_static void nnc_gen_label_decl(const nnc_quad* quad) {
    assert(quad->label != 0);
    GEN_TEXT(DR_LABEL"%u:\n", quad->label);
    nnc_reset_junk_vec();
}

nnc_static void nnc_gen_op_ref(const nnc_3a_quad* quad) {
    //todo: fix the case when lea has r/r operands
    nnc_loc dst_loc = nnc_store(&quad->res);
    nnc_gen_erase_reg(&dst_loc);
    GEN_INST(I_LEA);
    nnc_gen_loc(&dst_loc, quad->res.type);
    GEN_TEXT(",");
    nnc_gen_operand(&quad->arg1, quad->res.type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_op_cast(const nnc_3a_quad* quad) {
    nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
}

nnc_static void nnc_gen_op_copy(const nnc_quad* quad) {
    assert(!nnc_struct_or_union_type(quad->res.type));
    nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
}

nnc_static void nnc_gen_op_deref(const nnc_3a_quad* quad) {
    assert(!nnc_struct_or_union_type(quad->res.type));
    nnc_bool rbx_pushed = false;
    nnc_loc rbx_loc = nnc_mkloc_reg(R_RBX, &u64_type);
    if (!nnc_addr_inside_reg(&quad->arg1, R_RBX)) {
        rbx_pushed = nnc_gen_reserve_reg(R_RBX);
        nnc_gen_mov_operand_to_reg(&rbx_loc, &quad->arg1);
    }
    GEN_NOTE("--- deref:\n");
    rbx_loc.deref = true;
    rbx_loc.type = quad->res.type;
    nnc_gen_mov_reg_to_operand(&rbx_loc, &quad->res);
    nnc_gen_pop_reg(R_RBX, rbx_pushed);
}

nnc_static void nnc_gen_op_deref_copy(const nnc_3a_quad* quad) {
    nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
}

nnc_static void nnc_gen_op_unary(const nnc_3a_quad* quad, nnc_asm_inst inst) {
    GEN_INST(inst);
    //todo: come back here when optimizations will be presented
    assert(
        quad->arg1.kind == ADDR_CGT
    );
    nnc_gen_operand(&quad->arg1, quad->res.type);
    if (!nnc_cmp_addr(&quad->res, &quad->arg1)) {
        nnc_store(&quad->res);
        nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
    }
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_op_plus(const nnc_3a_quad* quad) {
    //GEN_NOTE("OP_PLUS here");
    //GEN_INST(I_NOP);
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

nnc_static void nnc_gen_op_binary(const nnc_3a_quad* quad, nnc_asm_inst inst) {
    GEN_INST(inst);
    assert(
        quad->arg1.kind == ADDR_CGT &&
        (
            quad->arg2.kind == ADDR_CGT ||
            quad->arg2.kind == ADDR_ICONST ||
            quad->arg2.kind == ADDR_NAME
        )
    );
    nnc_gen_operand(&quad->arg1, quad->res.type);
    GEN_TEXT(",");
    nnc_gen_operand(&quad->arg2, quad->res.type);
    GEN_TEXT("\r\n");
    // above code generates binary expressions in format: 
    //     arg1(res) = arg1 binOp arg2, if res == arg1
    // if res != arg1, mov instruction must be generated.
    if (!nnc_cmp_addr(&quad->res, &quad->arg1)) {
        nnc_store(&quad->res);
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
nnc_static void nnc_gen_one_op_mul(const nnc_3a_quad* quad, nnc_asm_inst mul) {
    nnc_bool rax_pushed = false;
    nnc_bool rdx_pushed = nnc_gen_reserve_reg(R_RDX);
    nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->arg1.type);
    nnc_u64 size = nnc_sizeof(quad->res.type);
    if (!nnc_addr_inside_reg(&quad->arg1, R_RAX)) {
        rax_pushed = nnc_gen_reserve_reg(R_RAX);
        nnc_gen_mov_operand_to_reg(&rax_loc, &quad->arg1);
    }
    GEN_INST(mul);
    nnc_gen_operand(&quad->arg2, quad->res.type);
    GEN_TEXT("\r\n");
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
nnc_static void nnc_gen_two_op_mul(const nnc_3a_quad* quad, nnc_asm_inst mul) {
    nnc_gen_op_binary(quad, mul);
}

/**
 * @brief Generates three operand multiplication instruction.
 *  See more at: https://www.felixcloutier.com/x86/imul#description
 * @param quad 
 * @param mul 
 * @return 
 */
nnc_static void nnc_gen_three_op_mul(const nnc_3a_quad* quad, nnc_asm_inst mul) {
    GEN_INST(mul);
    nnc_gen_operand(&quad->res, quad->res.type);
    GEN_TEXT(",");
    nnc_gen_operand(&quad->arg1, quad->res.type);
    GEN_TEXT(",");
    assert(quad->arg2.kind == ADDR_ICONST);
    nnc_gen_operand(&quad->arg2, quad->arg2.type);
    GEN_TEXT("\r\n");
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
        if (arg2_loc->where == L_REG   ||
            arg2_loc->where == L_DATA  ||
            arg2_loc->where == L_STACK) {
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
    if (nnc_signed_type(quad->res.type)) {
        nnc_gen_imul(quad);
    }
    else {
        nnc_gen_mul(quad);
    }
}

nnc_static void nnc_gen_rdx_extend(const nnc_3a_quad* quad) {
    switch (nnc_sizeof(quad->res.type)) {
        case PS_WORD:  GEN_INST(I_CWD); break;
        case PS_DWORD: GEN_INST(I_CDQ); break;
        case PS_QWORD: GEN_INST(I_CQO); break;
        default: {
            nnc_abort_no_ctx("nnc_GEN_INST_idiv: bad size of res.");
        }
    }
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_one_op_div(const nnc_3a_quad* quad, nnc_asm_inst div, nnc_bool mod) {
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
    if (glob_junk_vec[R_RDX] != REG_HAS_JUNK_0) {
        nnc_gen_xor_reg(&rdx_loc);
    }
    if (nnc_signed_type(quad->res.type)) {
        nnc_gen_rdx_extend(quad);
    }
    GEN_INST(div);
    nnc_gen_operand(&quad->arg2, quad->res.type);
    GEN_TEXT("\r\n");
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
    if (nnc_signed_type(quad->res.type)) {
        nnc_gen_div(quad, false);
    }
    else {
        nnc_gen_idiv(quad, false);
    }
}

nnc_static void nnc_gen_op_mod(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    if (nnc_signed_type(quad->res.type)) {
        nnc_gen_div(quad, true);
    }
    else {
        nnc_gen_idiv(quad, true);
    }
}

nnc_static void nnc_gen_inst_shift(const nnc_3a_quad* quad, nnc_asm_inst shift) {
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
    GEN_INST(shift);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    GEN_TEXT(",cl");
    // restore RCX if it was pushed
    nnc_gen_pop_reg(R_RCX, rcx_pushed);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_op_shr(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_inst_shift(quad, I_SHR);
}

nnc_static void nnc_gen_op_shl(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_inst_shift(quad, I_SHL);
}

nnc_static void nnc_gen_op_sal(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_inst_shift(quad, I_SAL);
}

nnc_static void nnc_gen_op_sar(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_inst_shift(quad, I_SAR);
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
    nnc_pclass_state* state = &buf_last(glob_call_states);
    // this is argument location, where the exact value stored
    nnc_loc loc = nnc_store(&quad->arg1);
    // this is the location where it is must be placed, according to ABI
    nnc_loc abi_loc = nnc_store_param(&quad->arg1, state);
    if (abi_loc.where == L_REG) {
        assert(loc.where == L_REG);
        nnc_gen_mov_reg_to_reg(&abi_loc, &loc);
        // todo: (fix) call function `nnc_gen_mov_operand_to_reg`
        // but note that `nnc_store_arg` updates current nnc_loc for quad->arg1,
        // so `nnc_store` inside of `nnc_gen_mov_operand_to_reg` will return same nnc_loc.
        buf_add(state->abi_used, abi_loc.exact.reg);
    }
    if (abi_loc.where == L_STACK) {
        assert(loc.where == L_REG);
        nnc_gen_push(&loc);
    }
}

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retp(const nnc_3a_quad* quad) {
    GEN_INST(I_JMP);
    GEN_TEXT(DR_RLABEL"%s\n", glob_asm_proc->name);
}

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retf(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->arg1.type));
    assert(quad->arg1.kind == ADDR_CGT);
    nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->res.type);
    nnc_gen_mov_operand_to_reg(&rax_loc, &quad->arg1);
    GEN_INST(I_JMP);
    GEN_TEXT(DR_RLABEL"%s\n", glob_asm_proc->name);
}

nnc_static void nnc_gen_call(const nnc_3a_addr* addr) {
    GEN_INST(I_CALL);
    if (addr->kind != ADDR_NAME) {
        nnc_gen_operand(addr, &u64_type);
    }
    else {
        assert(glob_ast != NULL);
        const char* fn = addr->exact.name.name;
        nnc_sym* sym = nnc_st_get_sym(glob_ast->st, fn);
        if (sym == NULL) {
            nnc_gen_operand(addr, &u64_type);
        }
        else {
            GEN_TEXT("%s", fn);
        }
    }
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_res_dealloc(nnc_pclass_state* state) {
    if (state->res == NULL) {
        return;
    }
    if (!nnc_addr_inside_reg(state->res, R_RAX)) {
        nnc_gen_pop_reg(R_RAX, state->was_res_pushed);
    }
}

nnc_static void nnc_gen_caller_prepare() {
    nnc_pclass_state* state = &buf_last(glob_call_states);
    nnc_u32 pad = nnc_get_stack_pad(glob_asm_proc->stack_alloc);
    pad += nnc_get_stack_pad(state->on_stack);
    state->on_stack += nnc_gen_align_stack(pad);
}

nnc_static void nnc_gen_caller_restore() {
    nnc_pclass_state* state = &buf_last(glob_call_states);
    nnc_gen_dealloc_stack(state->on_stack);
    for (nnc_u64 i = buf_len(state->pushed); i > 0; i--) {
        nnc_gen_pop(&REG_LOC(state->pushed[i-1]));
    }
    nnc_gen_res_dealloc(state);
    nnc_pclass_state_fini(state);
    buf_pop(glob_call_states);
}

nnc_static void nnc_gen_op_pcall(const nnc_3a_quad* quad) {
    nnc_gen_caller_prepare();
    nnc_gen_call(&quad->arg1);
    nnc_gen_caller_restore();
}

nnc_static void nnc_gen_op_fcall(const nnc_3a_quad* quad) {
    nnc_gen_caller_prepare();
    nnc_gen_call(&quad->arg1);
    nnc_loc res_loc = nnc_store(&quad->res);
    nnc_loc rax_loc = nnc_mkloc_reg(R_RAX, quad->res.type);
    nnc_gen_mov_reg_to_operand(&rax_loc, &quad->res);
    nnc_gen_caller_restore();
    //GEN_TEXT("# --------------------------------------------\n");
}

nnc_static void nnc_gen_cmp(const nnc_3a_quad* quad) {
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    const nnc_type* type = quad->arg1.type;
    if (nnc_sizeof(type) < nnc_sizeof(quad->arg2.type)) {
        type = quad->arg2.type;
    }
    GEN_INST(I_CMP);
    nnc_gen_operand(&quad->arg1, type);
    GEN_TEXT(",");
    nnc_gen_operand(&quad->arg2, type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_test(const nnc_3a_quad* quad) {
    assert(
        quad->arg1.kind == ADDR_CGT || 
        nnc_addr_inside(&quad->arg1, L_REG | L_MEM)
    );
    GEN_INST(I_TEST);
    nnc_gen_operand(&quad->arg1, quad->arg1.type);
    GEN_TEXT(",");
    nnc_gen_operand(&quad->arg1, quad->arg1.type);
    GEN_TEXT("\r\n");
}

nnc_static void nnc_gen_inst_jump(const nnc_3a_quad* quad, const char* jump) {
    GEN_INST(jump);
    nnc_gen_label(&quad->res);
}

nnc_static void nnc_gen_op_ujump(const nnc_3a_quad* quad) {
    nnc_gen_inst_jump(quad, I_JMP);
}

nnc_static void nnc_gen_op_cjumpt(const nnc_3a_quad* quad) {
    nnc_gen_test(quad);
    nnc_gen_inst_jump(quad, I_JNZ);
}

nnc_static void nnc_gen_op_cjumpf(const nnc_3a_quad* quad) {
    nnc_gen_test(quad);
    nnc_gen_inst_jump(quad, I_JZ);
}

nnc_static void nnc_gen_op_cjumplt(const nnc_3a_quad* quad) {
    nnc_gen_cmp(quad);
    nnc_gen_inst_jump(quad, nnc_signed_type(quad->res.type) ? I_JL : I_JB);
}

nnc_static void nnc_gen_op_cjumpgt(const nnc_3a_quad* quad) {
    nnc_gen_cmp(quad);
    nnc_gen_inst_jump(quad, nnc_signed_type(quad->res.type) ? I_JG : I_JA);
}

nnc_static void nnc_gen_op_cjumplte(const nnc_3a_quad* quad) {
    nnc_gen_cmp(quad);
    nnc_gen_inst_jump(quad, nnc_signed_type(quad->res.type) ? I_JLE : I_JBE);
}

nnc_static void nnc_gen_op_cjumpgte(const nnc_3a_quad* quad) {
    nnc_gen_cmp(quad);
    nnc_gen_inst_jump(quad, nnc_signed_type(quad->res.type) ? I_JGE : I_JAE);
}

nnc_static void nnc_gen_op_cjumpe(const nnc_3a_quad* quad) {
    if (quad->arg2.kind == ADDR_ICONST &&
        quad->arg2.exact.iconst.iconst == 0) {
        nnc_gen_test(quad);
    }
    else {
        nnc_gen_cmp(quad);
    }
    nnc_gen_inst_jump(quad, I_JE);
}

nnc_static void nnc_gen_op_cjumpne(const nnc_3a_quad* quad) {
    if (quad->arg2.kind == ADDR_ICONST &&
        quad->arg2.exact.iconst.iconst == 0) {
        nnc_gen_test(quad);
    }
    else {
        nnc_gen_cmp(quad);
    }
    nnc_gen_inst_jump(quad, I_JNE);
}

nnc_static void nnc_gen_op_decl_call(const nnc_3a_quad* quad) {
    nnc_pclass_state state = { .was_init = false };
    nnc_pclass_state_init(&state);
    //GEN_TEXT("# --------------------CALL--------------------\n");
    nnc_reg res_reg = R_NONE;
    if (quad->res.kind != ADDR_NONE) {
        // allocate location for call result
        nnc_loc res_loc = nnc_store(&quad->res);
        if (res_loc.where == L_REG) {
            res_reg = res_loc.exact.reg;
        }
    }
    // save used registers before call, but do not
    // save register that contains the result of the call,
    // otherwise it can cause erase call's result.
    for (nnc_reg r = R_RAX; r <= R_R15; r++) {
        if (nnc_reg_busy(r) && res_reg != r) {
            // add register to state `pushed` vector,
            // so they will be restored later. 
            nnc_push_reg(r), buf_add(state.pushed, r);
            nnc_gen_push(&REG_LOC(r));
        }
    }
    buf_add(glob_call_states, state);
}

nnc_static void nnc_gen_op_decl_local(const nnc_3a_quad* quad) {
    nnc_store(&quad->res);
    assert(nnc_get_loc(&quad->res)->where == L_STACK);
    assert(quad->res.exact.name.ictx == IDENT_DEFAULT);
}

nnc_static void nnc_gen_op_decl_global(const nnc_3a_quad* quad) {
    nnc_store(&quad->res);
    assert(nnc_get_loc(&quad->res)->where == L_DATA);
    assert(quad->res.exact.name.ictx == IDENT_GLOBAL);
}

nnc_static void nnc_gen_op_decl_string(const nnc_3a_quad* quad) {
    if (nnc_get_loc(&quad->res) == NULL) {
        nnc_loc loc = nnc_store(&quad->res);
        assert(loc.where == L_DATA);
        const nnc_str_literal* sl = (nnc_str_literal*)quad->hint;
        nnc_gen_mem(sl->exact, loc.exact.dsl, sl->bytes + 1);
    }
}

nnc_static void nnc_gen_proc_prologue(nnc_3a_proc* proc) {
    nnc_u32 pad = nnc_get_stack_pad(proc->l_stack);
    nnc_u32 num = proc->l_stack + pad;
    if (proc->l_stack != 0 && pad != 0) {
        GEN_TEXT("# stack size: %u;\n", proc->l_stack);
        GEN_TEXT("# aligned   : +%u;\n", pad);
    }
    if (proc->l_stack == 0) {
        // do not generate `enter` here because it will
        // contain `sub rsp, N` in it
        GEN_INST(I_PUSH), GEN_TEXT("rbp\n");
        GEN_INST(I_MOV),  GEN_TEXT("rbp,rsp\n");
    }
    else {
        GEN_INST(I_ENTER), GEN_TEXT("0x%x,0x0\n", num);
    }
}

nnc_static void nnc_gen_callee_prepare() {
    nnc_gen_proc_prologue(glob_asm_proc);
}

nnc_static void nnc_gen_proc_epilogue(nnc_3a_proc* proc) {
    GEN_INST(I_LEAVE), GEN_TEXT("\n");
}

nnc_static void nnc_gen_callee_restore() {
    GEN_TEXT(DR_RLABEL"%s:\n", glob_asm_proc->name);
    nnc_gen_proc_epilogue(glob_asm_proc);
    GEN_INST(I_RET), GEN_TEXT("\n\n");
}

nnc_static nnc_asm_proc nnc_build_proc(const nnc_blob_buf* code_impl) {
    nnc_asm_proc* proc = &glob_state.curr.asm_proc;
    if (FN_IS_PUB(glob_asm_proc)) {
        GEN_TEXT(DR_GLOBAL" %s\n", glob_asm_proc->name);
    }
    GEN_TEXT("%s:\n", glob_asm_proc->name);
    nnc_gen_callee_prepare();
    nnc_blob_buf_append(&proc->impl, code_impl);
    nnc_gen_callee_restore();
    return *proc;
}

nnc_static nnc_u32 nnc_get_callee_stack() {
    //todo: check if function has nested calls, to avoid useless spills
    // Due to the fact that all parameters are spilled 
    // to the stack at the beginning, allocated space
    return buf_len(glob_asm_proc->params) * PS_QWORD;
}

nnc_static void nnc_gen_store_params() {
    glob_asm_proc->p_stack = nnc_get_callee_stack();
    glob_asm_proc->p_stack_pad = nnc_get_stack_pad(glob_asm_proc->p_stack);
    nnc_pclass_state state = { .p_stack_pad = glob_asm_proc->p_stack_pad };
    nnc_pclass_state_init(&state);
    for (nnc_u64 i = 0; i < buf_len(glob_asm_proc->params); i++) {
        const nnc_fn_param* param = glob_asm_proc->params[i];
        const nnc_3a_addr addr = nnc_3a_mkname1(param->var);
        nnc_loc p_loc = nnc_store_param(&addr, &state);
        if (p_loc.where == L_REG) {
            nnc_loc p_mem_loc = nnc_spill(&addr);
            nnc_gen_mov_reg_to_mem(&p_mem_loc, &p_loc);
        }
    }
    assert(state.pushed == NULL);
    nnc_pclass_state_fini(&state);
}

/**
 * @brief Generates assembly for procedure declaration.
 * @param var Intermediate representation for procedure declaration. 
 */
nnc_static void nnc_gen_proc(nnc_ir_proc* proc) {
    // reset current assembly procedure for generation
    glob_state.curr.asm_proc = (nnc_asm_proc){
        .code = proc,
        .impl = {0}
    };
    // initialize implementation of the procedure
    // this is done just by initializing it's blob buffer
    nnc_blob_buf_init(&glob_state.curr.asm_proc.impl);
    // reset callee
    nnc_reset_lr_vec();
    nnc_reset_loc_map();
    nnc_reset_junk_vec();
    // store and spill params
    nnc_gen_store_params();
    nnc_u64 q_len = buf_len(proc->quads);
    for (nnc_u64 i = 0; i < q_len; i++) {
        const nnc_quad* quad = &proc->quads[i];
        if (quad->label != 0) {
            nnc_gen_label_decl(quad);
        }
        else {
            //todo: may be move this to separate function,
            // but now there are less calls..
            switch (quad->op) {
                /* Other operators */
                case OP_REF:        nnc_gen_op_ref(quad);        break;
                case OP_CAST:       nnc_gen_op_cast(quad);       break;
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
                case OP_HINT_DECL_CALL:   nnc_gen_op_decl_call(quad);   break;
                case OP_HINT_DECL_LOCAL:  nnc_gen_op_decl_local(quad);  break;
                case OP_HINT_DECL_GLOBAL: nnc_gen_op_decl_global(quad); break;
                case OP_HINT_DECL_STRING: nnc_gen_op_decl_string(quad); break;
                default: {
                   nnc_abort_no_ctx("nnc_gen_op: unknown operator.\n");
                }
            }
        }
        proc->quad_pointer++;
    }
    nnc_blob_buf code_impl = glob_state.curr.asm_proc.impl;
    glob_state.curr.asm_proc.impl = (nnc_blob_buf){0};
    nnc_blob_buf_init(&glob_state.curr.asm_proc.impl);
    nnc_build_proc(&code_impl);
    buf_add(glob_asm_file.procs, glob_state.curr.asm_proc);
    nnc_blob_buf_fini(&code_impl);
}

/**
 * @brief Generates assembly for global ir symbol 
 *  (procedure or global variable for now).
 * @param ir Global ir symbol to be generated. 
 * @return Returns assembly file representation 
 *  which contains generated ir symbol. 
 */
nnc_asm_file* nnc_gen(nnc_ir_sym ir_sym) {
    if (ir_sym.kind == STMT_LET) {
        nnc_gen_var(&ir_sym.sym.var);
    }
    if (ir_sym.kind == STMT_FN) {
        nnc_gen_proc(&ir_sym.sym.proc);
    }
    return &glob_asm_file;
}

nnc_blob_buf nnc_build(nnc_asm_file* file) {
    nnc_blob_buf impl = (nnc_blob_buf){0};
    nnc_blob_buf_init(&impl);
    nnc_blob_buf_putf(&impl,
        DR_ISYNTAX" noprefix \n"
        "\n"
        DR_TEXT"\n"
        "\n"
    );
    for (nnc_u64 i = 0; i < buf_len(glob_asm_file.procs); i++) {
        nnc_blob_buf_append(&impl, &glob_asm_file.procs[i].impl);
        nnc_blob_buf_fini(&glob_asm_file.procs[i].impl);
    }
    if (glob_asm_file.ds_impl.len != 0) {
        nnc_blob_buf_putf(&impl, "\n"DR_DATA"\n\n");
        nnc_blob_buf_append(&impl, &glob_asm_file.ds_impl);
        nnc_blob_buf_fini(&glob_asm_file.ds_impl);
    }
    nnc_reset_str_map();
    return impl;
}