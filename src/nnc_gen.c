#include "nnc_gen.h"
#include "nnc_state.h"

#define REG_HAS_JUNK_0  (0)
#define REG_HAS_JUNK_8  (1)
#define REG_HAS_JUNK_16 (2)
#define REG_HAS_JUNK_32 (4)
#define REG_HAS_JUNK_64 (8)

#define S_QWORD 8
#define S_DWORD 4
#define S_WORD  2
#define S_BYTE  1

#define NNC_STACK_ALIGN 16

#define NNC_RT_PATH   "nnc_rt.o"
#define NNC_CRT1_PATH "crt1.o"
#define NNC_CRTi_PATH "crti.o"
#define NNC_CRTn_PATH "crtn.o"

#define NNC_MEM_PAD_SIZE 8

#define NNC_LABEL_PREFIX     "_l"
#define NNC_RET_LABEL_PREFIX "_r"

#define gen_note(...)

#define glob_quad glob_asm_proc->quads[glob_asm_proc->quad_pointer]

nnc_static nnc_u8 glob_reg_junk_level[21] = {0};

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
            case S_BYTE:  return _int[reg][0];
            case S_WORD:  return _int[reg][1];
            case S_DWORD: return _int[reg][2];
            case S_QWORD: return _int[reg][3];
            default: {
                nnc_abort_no_ctx("nnc_get_reg_of_size: unsupported size.\n");
            }
        }
    }
    return NULL;
}

nnc_static const char* nnc_get_reg_of_type(nnc_reg reg, const nnc_type* type) {
    if (nnc_arr_or_ptr_type(type)) {
        return nnc_get_reg_of_size(reg, S_QWORD);
    }
    if (nnc_real_type(type)) {
        return nnc_get_reg_of_size(reg, S_QWORD);
    }
    else {
        return nnc_get_reg_of_size(reg, nnc_sizeof(type));
    }
}

nnc_static void nnc_pollute(nnc_reg reg, nnc_u8 amount) {
    assert(
        amount == 0 ||
        amount == 1 ||
        amount == 2 ||
        amount == 4 ||
        amount == 8
    );
    glob_reg_junk_level[reg] = amount;
}

nnc_static nnc_bool nnc_gen_reserve_reg(nnc_reg reg) {
    nnc_bool need_push = nnc_reserve_reg(reg);
    if (need_push) {
        text_put("push %s\n", glob_reg_str[reg]);
    }
    return need_push;
}

nnc_static void nnc_gen_pop_reg(nnc_reg reg, nnc_bool pushed) {
    nnc_unreserve_reg(reg);
    if (pushed) {
        text_put("pop %s\n", glob_reg_str[reg]);
        nnc_pollute(reg, REG_HAS_JUNK_64);
    }
}

nnc_static void nnc_gen_unreserve_reg(nnc_reg reg) {
    nnc_gen_pop_reg(reg, true);
}

nnc_static void nnc_gen_xor_reg(const nnc_loc* at) {
    assert(at && at->where == L_REG);
    nnc_u64 junk = glob_reg_junk_level[at->exact.reg];
    if (junk != REG_HAS_JUNK_0) {
        const char* reg_str = nnc_get_reg_of_size(at->exact.reg, junk);
        text_put("xor %s,%s\n", reg_str, reg_str);
    }
    nnc_pollute(at->exact.reg, REG_HAS_JUNK_0);
}

nnc_static void nnc_gen_erase_reg(const nnc_loc* at) {
    assert(at && at->where == L_REG);
    nnc_u64 size = REG_HAS_JUNK_64;
    nnc_u64 junk = glob_reg_junk_level[at->exact.reg];
    if (!nnc_arr_type(at->type)) {
        size = nnc_sizeof(at->type);
    }
    // make it not generate xor when trashed by 64bits, but write 32bits.
    if (junk != REG_HAS_JUNK_0  &&
        junk == REG_HAS_JUNK_64 && size == S_DWORD) {
        nnc_pollute(at->exact.reg, REG_HAS_JUNK_32);
    }
    else {
        if (junk > size) {
            gen_note("---- reg %s has %d junked byte(s)\n", glob_reg_str[at->reg], junk);
            nnc_gen_xor_reg(at);
        }
        nnc_pollute(at->exact.reg, size);
    }
}

nnc_static void nnc_gen_loc(const nnc_loc* at, const nnc_type* type) {
    switch (at->where) {
        case L_REG: {
            if (at->deref) {
                const char* ptr_size = nnc_get_ptr_size(type);
                text_put("%s ptr [%s]", ptr_size, glob_reg_str[at->exact.reg]);
            }
            else {
                text_put("%s", nnc_get_reg_of_type(at->exact.reg, type));
            }
            break;
        }
        case L_STACK: {
            const char* ptr_size = nnc_get_ptr_size(type);
            text_put("%s ptr [rbp-%ld]", ptr_size, at->exact.mem);
            break;
        }
        case L_DATA: {
            if (at->has_offset) {
                text_put("offset ");    
            }
            if (at->deref) {
                const char* ptr_size = nnc_get_ptr_size(type);
                text_put("%s ptr [%s]", ptr_size, at->exact.dsl);
            }
            else {
                text_put("%s", at->exact.dsl);
            }
            break;
        }
        case L_IMM: {
            text_put("0x%lx", at->exact.imm.d);
            break;
        }
        default: {
            nnc_abort_no_ctx("nnc_gen_storage: unsupported storage type.\n");
        }
    }
}

nnc_static void nnc_gen_mov_mem_to_reg(const nnc_loc* reg, const nnc_loc* mem) {
    assert(mem->where & L_MEM);
    nnc_loc dst_loc = *reg;
    nnc_loc src_loc = *mem;
    gen_note("--- mov memory to reg:\n");
    const char* i_mov = "mov ";
    if (glob_quad.op == OP_CAST) {
        return;
        //if (nnc_gen_conv(&dst_loc, &src_loc)) {
        //    return;
        //}
    }
    if (nnc_arr_type(src_loc.type)) {
        if (src_loc.where == L_DATA) {
            src_loc.has_offset = true;
        }
        else {
            i_mov = "lea ";
        }
    }
    nnc_gen_erase_reg(&dst_loc);
    text_put(i_mov);
    nnc_gen_loc(&dst_loc, src_loc.type);
    text_put(",");
    nnc_gen_loc(&src_loc, src_loc.type);
    text_put("\r\n");
}

nnc_static void nnc_gen_reg_to_mem_mov(const nnc_loc* reg, const nnc_loc* mem) {
    assert(reg->where == L_REG);
    assert(mem->where & L_MEM);
    gen_note("--- mov reg to memory:\n");
    //todo: maybe not regular mov?
    text_put("mov ");
    nnc_gen_loc(mem, reg->type);
    text_put(",");
    text_put("%s", nnc_get_reg_of_type(reg->exact.reg, reg->type));
    text_put("\r\n");
}

nnc_static void nnc_gen_mov_reg_to_reg(const nnc_loc* dst_reg, const nnc_loc* src_reg) {
    assert(src_reg->where == L_REG);
    nnc_loc src_loc = *src_reg;
    nnc_loc dst_loc = *dst_reg;
    gen_note("--- mov reg to reg:\n");
    gen_note("----- dst reg junked by %d:\n", glob_reg_junk_level[dst_loc.reg]);
    if (glob_quad.op == OP_CAST) {
        return;
        //if (nnc_gen_conv(&dst_loc, &src_loc)) {
        //    return;
        //}
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
    text_put("mov ");
    nnc_gen_loc(&dst_loc, dst_loc.type);
    text_put(",");
    nnc_gen_loc(&src_loc, dst_loc.type);
    text_put("\r\n");
}

nnc_static void nnc_gen_mov_imm_to_reg(const nnc_loc* reg, const nnc_loc* imm) {
    assert(reg->where == L_REG);
    assert(imm->where == L_IMM);
    gen_note("--- mov imm to reg:\n");
    nnc_gen_erase_reg(reg);
    text_put("mov ");
    nnc_gen_loc(reg, reg->type);
    text_put(",");
    nnc_gen_loc(imm, reg->type);
    text_put("\r\n");
}

nnc_static void nnc_gen_mov_reg_to_mem(const nnc_loc* mem, const nnc_loc* reg) {
    assert(reg->where == L_REG);
    assert(mem->where & L_MEM);
    gen_note("--- mov reg to memory:\n");
    text_put("mov ");
    nnc_gen_loc(mem, mem->type);
    text_put(",");
    nnc_gen_loc(reg, mem->type);
}

nnc_static void nnc_gen_mov_mem_to_mem(const nnc_loc* dst_mem, const nnc_loc* src_mem) {
    assert(src_mem->where & L_MEM);
    assert(dst_mem->where & L_MEM);
    gen_note("--- mov memory to memory:\n");
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
    text_put("mov ");
    nnc_gen_loc(mem, mem->type);
    text_put(",");
    nnc_gen_loc(imm, mem->type);
}

nnc_static void nnc_gen_mov_operand_to_reg(const nnc_loc* reg, const nnc_3a_addr* src) {
    assert(reg->where == L_REG);
    nnc_loc src_loc = nnc_store(src);
    gen_note("--- mov operand to reg:\n");
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
    gen_note("--- mov reg to operand:\n");
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
    gen_note("--- mov operand to memory:\n");
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
        gen_note("--- mov operand to (deref) operand:\n");
        dst_loc.deref = true;
        dst_loc.type = src->type;
    }
    else {
        gen_note("--- mov operand to operand:\n");
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

/**
 * @brief Generates initialized memory on data segment. 
 * @param mem Memory for initialization.
 * @param name Name of the memory span.
 * @param size Size of memory span in bytes.
 */
nnc_static void nnc_gen_mem(const char* mem, const char* name, nnc_u64 size) {
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

/**
 * @brief Generates zeroed memory pad on data segment.
 * @param size Size of memory pad in bytes.
 */
nnc_static void nnc_gen_mem_pad(nnc_u64 size) {
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

/**
 * @brief Generates named memory span on data segment (all zeros).
 * @param name Name of the memory span (label).
 * @param size Size of memory span in bytes.
 */
nnc_static void nnc_gen_mem_named(const char* name, nnc_u64 size) {
    data_put("   %s:", name);
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

nnc_static void nnc_gen_label(const nnc_quad* quad) {
    assert(quad->label != 0);
    text_put(NNC_LABEL_PREFIX"%u:", quad->label);
}

nnc_static void nnc_gen_op_copy(const nnc_quad* quad) {
    assert(!nnc_struct_or_union_type(quad->res.type));
    nnc_gen_mov_operand_to_operand(&quad->res, &quad->arg1);
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
    // calculate amount of stack space to be added
    // for performing 16byte stack alignment required by the ABI
    nnc_u32 pad = NNC_STACK_ALIGN - (proc->local_stack_offset % NNC_STACK_ALIGN);
    nnc_u32 num = proc->local_stack_offset + pad;
    if (proc->local_stack_offset == 0) {
        // do not generate `enter` here because it will
        // contain `sub rsp, N` in it
        text_put("push rbp\n");
        text_put("mov rbp,rsp\n");
    }
    else {
        text_put("enter %u,0\n", num);
    }
}

nnc_static void nnc_gen_callee_prepare() {
    nnc_gen_proc_prologue(glob_asm_proc);
}

nnc_static void nnc_gen_proc_epilogue(nnc_3a_proc* proc) {
    text_put("leave\n");
}

nnc_static void nnc_gen_callee_restore() {
    text_put(NNC_RET_LABEL_PREFIX"%s:\n", glob_asm_proc->name);
    nnc_gen_proc_epilogue(glob_asm_proc);
    text_put("ret\n\n");
}

nnc_static nnc_asm_proc nnc_build_proc(const nnc_blob_buf* code_impl) {
    nnc_asm_proc* proc = &glob_state.curr.asm_proc;
    if (FN_IS_PUB(glob_asm_proc)) {
        text_put(".global %s\n", glob_asm_proc->name);
    }
    text_put("%s:\n", glob_asm_proc->name);
    nnc_gen_callee_prepare();
    nnc_blob_buf_append(&proc->impl, code_impl);
    nnc_gen_callee_restore();
    return *proc;
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
    // todo: callee reset...
    // todo: store params...
    // todo: spill params...
    nnc_u64 q_len = buf_len(proc->quads);
    for (nnc_u64 i = 0; i < q_len; i++) {
        const nnc_quad* quad = &proc->quads[i];
        if (quad->label != 0) {
            nnc_gen_label(quad);
        }
        else {
            //todo: may be move this to separate function,
            // but now there are less calls..
            switch (quad->op) {
                /* Other operators */
                //case OP_REF:        nnc_gen_op_ref(quad);        break;
                //case OP_CAST:       nnc_gen_op_cast(quad);       break;
                case OP_COPY:       nnc_gen_op_copy(quad);       break;
                //case OP_DEREF:      nnc_gen_op_deref(quad);      break;
                //case OP_DEREF_COPY: nnc_gen_op_deref_copy(quad); break;
                ///* Unary operators */
                //case OP_PLUS:   nnc_gen_op_plus(quad);  break;
                //case OP_MINUS:  nnc_gen_op_minus(quad); break;
                //case OP_BW_NOT: nnc_gen_op_not(quad);   break;
                ///* Binary operators */
                //case OP_ADD:    nnc_gen_op_add(quad); break;
                //case OP_SUB:    nnc_gen_op_sub(quad); break;
                //case OP_MUL:    nnc_gen_op_mul(quad); break;
                //case OP_DIV:    nnc_gen_op_div(quad); break;
                //case OP_MOD:    nnc_gen_op_mod(quad); break;
                //case OP_SHR:    nnc_gen_op_shr(quad); break;
                //case OP_SHL:    nnc_gen_op_shl(quad); break;
                //case OP_SAL:    nnc_gen_op_sal(quad); break;
                //case OP_SAR:    nnc_gen_op_sar(quad); break;
                //case OP_BW_OR:  nnc_gen_op_or(quad);  break;
                //case OP_BW_AND: nnc_gen_op_and(quad); break;
                //case OP_BW_XOR: nnc_gen_op_xor(quad); break;
                ///* Other operators */
                //case OP_ARG:   nnc_gen_op_arg(quad);   break;
                //case OP_RETP:  nnc_gen_op_retp(quad);  break;
                //case OP_RETF:  nnc_gen_op_retf(quad);  break;
                //case OP_PCALL: nnc_gen_op_pcall(quad); break;
                //case OP_FCALL: nnc_gen_op_fcall(quad); break;
                ///* Conditional operators */
                //case OP_UJUMP:    nnc_gen_op_ujump(quad);    break;
                //case OP_CJUMPT:   nnc_gen_op_cjumpt(quad);   break;
                //case OP_CJUMPF:   nnc_gen_op_cjumpf(quad);   break;
                //case OP_CJUMPLT:  nnc_gen_op_cjumplt(quad);  break;
                //case OP_CJUMPGT:  nnc_gen_op_cjumpgt(quad);  break;
                //case OP_CJUMPLTE: nnc_gen_op_cjumplte(quad); break; 
                //case OP_CJUMPGTE: nnc_gen_op_cjumpgte(quad); break;
                //case OP_CJUMPE:   nnc_gen_op_cjumpe(quad);   break;
                //case OP_CJUMPNE:  nnc_gen_op_cjumpne(quad);  break;
                ///* Hint operators */
                //case OP_HINT_DECL_LOCAL:       nnc_gen_op_decl_local(quad);  break;
                case OP_HINT_DECL_STRING:      nnc_gen_op_decl_string(quad); break;
                //case OP_HINT_PREPARE_FOR_CALL: nnc_gen_op_prepare_for_call(quad); break;
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
        ".intel_syntax noprefix \n"
        "\n"
        ".text\n"
        "\n"
    );
    for (nnc_u64 i = 0; i < buf_len(glob_asm_file.procs); i++) {
        nnc_blob_buf_append(&impl, &glob_asm_file.procs[i].impl);
        nnc_blob_buf_fini(&glob_asm_file.procs[i].impl);
    }
    if (glob_asm_file.ds_impl.len != 0) {
        nnc_blob_buf_putf(&impl, "\n.data\n");
        nnc_blob_buf_append(&impl, &glob_asm_file.ds_impl);
        nnc_blob_buf_fini(&glob_asm_file.ds_impl);
    }
    return impl;
}