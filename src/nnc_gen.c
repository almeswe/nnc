#include "nnc_gen.h"

#define S_QWORD 8
#define S_DWORD 4
#define S_WORD  2
#define S_BYTE  1

nnc_static void nnc_blob_buf_init(nnc_blob_buf* buf) {
    assert(buf->len == 0);
    assert(buf->cap == 0);
    assert(buf->blob == NULL);
    buf->len = 0;
    buf->cap = FMEMOPEN_BUF_GROWTH;
    buf->blob = (char*)nnc_alloc(buf->cap);
}

nnc_static void nnc_blob_buf_fini(nnc_blob_buf* buf) {
    assert(buf->blob != NULL);
    nnc_dispose(buf->blob);
}

nnc_static void nnc_blob_buf_grow(nnc_blob_buf* buf) {
    assert(buf->blob != NULL);
    nnc_blob_buf prev_buf = *buf;
    buf->cap += FMEMOPEN_BUF_GROWTH;
    buf->blob = (char*)nnc_alloc(buf->cap);
    memcpy(buf->blob, prev_buf.blob, prev_buf.cap);
    nnc_dispose(prev_buf.blob);
}

nnc_static void nnc_blob_buf_append(nnc_blob_buf* src, const nnc_blob_buf* buf) {
    nnc_blob_buf prev_src = *src;
    src->cap += buf->cap;
    src->len += buf->len;
    src->blob = (char*)nnc_alloc(src->cap);
    memcpy(src->blob, prev_src.blob, prev_src.len);
    memcpy(src->blob + prev_src.len, buf->blob, buf->len);
    nnc_blob_buf_fini(&prev_src);
}

nnc_static void nnc_blob_buf_putf(nnc_blob_buf* buf, const char* format, ...) {
    assert(buf->cap != 0);
    assert(buf->blob != NULL);
    va_list args;
	va_start(args, format);
    char formatbuf[FMEMOPEN_BUF_GROWTH] = {0};
    nnc_u64 written = vsnprintf(formatbuf, sizeof formatbuf, format, args);
	va_end(args);
    if (written + buf->len >= buf->cap) {
        nnc_blob_buf_grow(buf);
    }
    memcpy(buf->blob + buf->len, formatbuf, written);
    buf->len += written;
}

#define SEG_TEXT (&glob_current_asm_proc.impl)
#define SEG_DATA (&glob_current_asm_file.data_segment_impl)

#define data_put(...) nnc_blob_buf_putf(SEG_DATA, __VA_ARGS__)
#define text_put(...) nnc_blob_buf_putf(SEG_TEXT, __VA_ARGS__)

nnc_assembly_file glob_current_asm_file = {0};
nnc_assembly_proc glob_current_asm_proc = {0};

nnc_3a_proc* glob_current_unit = NULL;

#define gen_t0(...)  text_put(__VA_ARGS__)
#define gen_t1(...)  text_put("   " __VA_ARGS__)
#define gen_instr(i) gen_t1("%s ", nnc_asm_instr_str[i])
#define gen_reg(r)   gen_t0("%s", glob_reg_str[(r)])
#define gen_comma()  gen_t0(", ")
#define gen_crlf()   text_put("\r\n")

#define NNC_LABEL_PREFIX     "label_"
#define NNC_RET_LABEL_PREFIX "label_ret_"

nnc_static void nnc_gen_push_reg(nnc_register reg) {
    nnc_push_reg(reg);
    gen_t1("push %s\n", glob_reg_str[reg]);
}

nnc_static nnc_bool nnc_gen_reserve_reg(nnc_register reg) {
    nnc_bool need_push = nnc_reserve_reg(reg);
    if (need_push) {
        gen_instr(I_PUSH);
        gen_t0("%s\n", glob_reg_str[reg]);
    }
    return need_push;
}

nnc_static void nnc_gen_unreserve_reg(nnc_register reg) {
    nnc_unreserve_reg(reg);
    gen_instr(I_POP);
    gen_t0("%s\n", glob_reg_str[reg]);
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

/**
 * @brief Checks if `addr` can be inlined as instructions operand,
 *  or it must be passed in another way. (via register or memory for example) 
 * @param addr 
 * @param allowed Allowed operand modes. (immediate, register, memory)
 * @return 
 */
//nnc_static nnc_bool nnc_able_to_inline_addr(const nnc_3a_addr* addr, nnc_u8 allowed) {
//    switch (addr->kind) {
//        case ADDR_FCONST:
//        case ADDR_ICONST: {
//            return (allowed & A_IMM) != 0;
//        }
//        case ADDR_NAME: {
//            return (allowed & A_MEM) != 0;
//        }
//        default: return false;
//    }
//}

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

nnc_static nnc_3a_lr* nnc_get_lr(nnc_3a_proc* unit, const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:  return (nnc_3a_lr*)map_get(unit->lr_cgt, addr->exact.cgt); 
        case ADDR_NAME: return (nnc_3a_lr*)map_get_s(unit->lr_var, addr->exact.name.name);
        default: {
            nnc_abort_no_ctx("bad addr kind\n");
        }
    }
    return NULL;
}

nnc_static nnc_asm_operand nnc_get_operand_type(const nnc_3a_addr* addr) {
    nnc_asm_operand op_type = 0;
    switch (addr->kind) {
        case ADDR_CGT:
        case ADDR_NAME: {
            const nnc_loc* loc = nnc_get_loc(addr);
            assert(loc != NULL);
            //assert(
            //    loc->where == LOCATION_REG  ||
            //    loc->where == LOCATION_DATA || 
            //    loc->where == LOCA
            //);
            switch (loc->where) {
                case L_REG:   op_type = A_REG8; break;
                case L_DATA:  op_type = A_MEM8; break;
                case L_LOCAL_STACK: op_type = A_MEM8; break;
                case L_PARAM_STACK: op_type = A_MEM8; break;
                default: {
                    nnc_abort_no_ctx("nnc_get_operand_type: unknown storage.\n");
                }
            }
            switch (addr->type->size) {
                case S_BYTE:  return op_type;
                case S_WORD:  return op_type << 1;
                case S_DWORD: return op_type << 2;
                case S_QWORD: return op_type << 3;
                default: {
                    nnc_abort_no_ctx("nnc_get_operand_type: unsupported size.\n");
                }
            }
            break;
        }
        case ADDR_ICONST: {
            op_type = A_IMM8;
            nnc_u64 imm = addr->exact.iconst.iconst;
            if (imm > UINT8_MAX) {
                op_type = A_IMM16;
            }
            if (imm > UINT16_MAX) {
                op_type = A_IMM32;
            }
            if (imm > UINT32_MAX) {
                op_type = A_IMM64;
            }
            break;
        }
        //case ADDR_FCONST: nnc_gen_addr_fconst(addr); break;
        default: {
            nnc_abort_no_ctx("nnc_get_operand_type: unknown addr kind.\n");
        }
    }
    return op_type;
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
    return (loc->where & at) != 0;
}

nnc_static void nnc_gen_loc(const nnc_loc* at, const nnc_type* type) {
    switch (at->where) {
        case L_REG: {
            gen_t0("%s", nnc_get_reg_of_size(at->reg, type));
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
        default: {
            nnc_abort_no_ctx("nnc_gen_storage: unsupported storage type.\n");
        }
    }
}

nnc_static void nnc_gen_addr_cgt(const nnc_3a_addr* addr) {
    nnc_loc at = nnc_store(addr, false);
    nnc_gen_loc(&at, addr->type);
}

nnc_static void nnc_gen_addr_var(const nnc_3a_addr* addr) {
    nnc_loc at = nnc_store(addr, false);
    nnc_gen_loc(&at, addr->type);
}

nnc_static void nnc_gen_addr_iconst(const nnc_3a_addr* addr) {
    if (nnc_integral_type(addr->type)) {
        gen_t0("%ld", (nnc_i64)addr->exact.iconst.iconst);
    }
    else {
        gen_t0("%lu", (nnc_u64)addr->exact.iconst.iconst);
    }
}

nnc_static void nnc_gen_addr_sconst(const nnc_3a_addr* addr) {
    nnc_loc at = nnc_store(addr, true);
    nnc_gen_loc(&at, addr->type);
    data_put("   %s:", at.ds_name);
    nnc_u64 len = strlen(addr->exact.sconst.sconst);
    for (nnc_u64 i = 0, b = 0; i < len; i++, b++) {
        if (b % 8 == 0) {
            data_put("\n.byte ");
            b = 0;
        }
        data_put("0x%x", addr->exact.sconst.sconst[i]);
        if (b != 7) {
            data_put(", ");
        }
    }
    data_put("0x0\n");
}

nnc_static void nnc_gen_addr_fconst(const nnc_3a_addr* addr) {
    nnc_abort_no_ctx("nnc_gen_addr_fconst: is not implemented.\n");    
}

nnc_static void nnc_gen_3a_addr(const nnc_loc* at, const nnc_3a_addr* addr) {
    // in this case check if storage is presented as direct store
    // it means that `nnc_3a_addr` is direct instruction argument 
    if (at != NULL && at->where != L_NONE) {
        gen_instr(I_MOV);
        nnc_gen_loc(at, addr->type);
        gen_comma();
    }
    switch (addr->kind) {
        case ADDR_CGT:    nnc_gen_addr_cgt(addr);    break;
        case ADDR_NAME:   nnc_gen_addr_var(addr);    break;
        case ADDR_ICONST: nnc_gen_addr_iconst(addr); break;
        case ADDR_FCONST: nnc_gen_addr_fconst(addr); break;
        case ADDR_SCONST: nnc_gen_addr_sconst(addr); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op: unknown addr kind.\n");
        }
    }
}

nnc_static void nnc_gen_operand(const nnc_3a_addr* addr, const nnc_type* type) {
    if (addr->kind == ADDR_CGT || addr->kind == ADDR_NAME) {
        nnc_loc loc = nnc_store(addr, false);
        nnc_gen_loc(&loc, type);
    }
    else {
        nnc_gen_3a_addr(NULL, addr);                
    }
}

nnc_static void nnc_gen_mem_to_reg_mov(nnc_register reg, const nnc_loc* mem) {
    assert(mem->where & LOCATION_MEM);
    assert(nnc_pow2(nnc_sizeof(mem->type)));
    nnc_asm_instr i_mov = I_MOV;
    if (nnc_sizeof(mem->type) < S_QWORD) {
        i_mov = nnc_signed_type(mem->type) ? I_MOVSX : I_MOVZX;
    }
    gen_instr(i_mov);
    gen_reg(reg);
    gen_comma();
    nnc_gen_loc(mem, mem->type);
    gen_crlf();
}

nnc_static void nnc_gen_reg_to_reg_mov(nnc_register dst_reg, const nnc_loc* src_reg) {
    assert(src_reg->where == L_REG);
    assert(nnc_pow2(nnc_sizeof(src_reg->type)));
    nnc_asm_instr i_mov = I_MOV;
    if (nnc_sizeof(src_reg->type) < S_QWORD) {
        i_mov = nnc_signed_type(src_reg->type) ? I_MOVSX : I_MOVZX;
    }
    else {
        // if registers are the same and both 8 bytes,
        // skip, because copy from the same reg is seamless
        if (src_reg->reg == dst_reg) {
            return;
        }
    }
    gen_instr(i_mov);
    gen_reg(dst_reg);
    gen_comma();
    nnc_gen_loc(src_reg, src_reg->type);
    gen_crlf();
}

nnc_static void nnc_gen_operand_to_reg_mov(nnc_register reg, const nnc_3a_addr* addr) {
    nnc_loc loc = nnc_store(addr, false);
    if (loc.where & LOCATION_MEM) {
        nnc_gen_mem_to_reg_mov(reg, &loc);
    }
    else {
        assert(loc.where == L_REG);
        nnc_gen_reg_to_reg_mov(reg, &loc);
    }
}

nnc_static void nnc_gen_mov_reg_to_operand(nnc_register reg, 
    const nnc_3a_addr* addr, const nnc_type* type) {
    gen_instr(I_MOV);
    nnc_gen_operand(addr, type);
    gen_comma();
    gen_t0("%s", nnc_get_reg_of_size(reg, type));
    gen_crlf();
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

nnc_static void nnc_gen_generic_mov(const nnc_3a_addr* dst, const nnc_3a_addr* src) {
    //todo: extend this code
    assert(!nnc_real_type(dst->type));
    assert(!nnc_struct_or_union_type(dst->type));
    nnc_loc dst_loc = nnc_store(dst, false);
    nnc_loc src_loc = nnc_store(src, false);
    //if (dst_loc.where == LOCATION_REG && (src_loc.where & LOCATION_MEM)) {
    //    assert(false)''
    //    //nnc_gen_mem_to_reg_mov(&dst_loc, &src_loc);
    //}
    //else {
        gen_instr(I_MOV);
        nnc_gen_operand(dst, dst->type);
        gen_comma();
        nnc_gen_operand(src, dst->type);
    //}
}

nnc_static void nnc_gen_derefd_reg(nnc_register reg, const nnc_type* type) {
    gen_t0("%s ptr [%s]", nnc_get_ptr_size(type), glob_reg_str[reg]);
}

nnc_static void nnc_gen_derefd_operand(const nnc_3a_addr* addr, const nnc_type* type) {
    nnc_loc loc = nnc_store(addr, false);
    if (loc.where == L_REG) {
        nnc_gen_derefd_reg(loc.reg, type);
    }
    else {
        nnc_gen_loc(&loc, type);
    }
}

nnc_static void nnc_gen_op_deref(const nnc_3a_quad* quad) {
    assert(!nnc_struct_or_union_type(quad->res.type));
    //todo: why each time RBX is pushed?
    nnc_bool rbx_pushed = nnc_gen_reserve_reg(R_RBX);
    nnc_gen_operand_to_reg_mov(R_RBX, &quad->arg1);
    //if (!nnc_addr_inside_reg(&quad->arg1, R_RBX)) {
    //    //nnc_gen_generic_mov();
    //    gen_t1("mov rbx, ");
    //    const nnc_loc* loc = nnc_get_loc(&quad->arg1);
    //    assert(loc != NULL);
    //    nnc_gen_loc(loc, &u64_type);
    //    gen_crlf();
    //}
    //todo: SEGFAULT??
    //todo: fully implement `nnc_gen_generic_mov` and same kind of functions, especially `nnc_gen_mov_operand_to_reg` etc.
    //todo: refactor this
    nnc_asm_instr i_mov = I_MOV;
    if (quad->res.type->size < 8) {
        i_mov = nnc_signed_type(quad->res.type) ? I_MOVSX : I_MOVZX;
    }
    gen_instr(i_mov);
    nnc_gen_operand(&quad->res, &u64_type);
    gen_comma();
    nnc_gen_derefd_reg(R_RBX, quad->res.type);
    if (rbx_pushed) {
        gen_crlf();
        nnc_gen_unreserve_reg(R_RBX);
    }
}

nnc_static void nnc_gen_op_copy(const nnc_3a_quad* quad) {
    nnc_asm_instr i_mov = I_MOV;
    if (nnc_real_type(quad->res.type)) {
        i_mov = I_MOVSS;
        if (quad->res.type->kind == T_PRIMITIVE_F64) {
            i_mov = I_MOVSD;
        }
    }
    if (nnc_arr_type(quad->res.type)) {
        nnc_loc loc = nnc_store(&quad->arg1, false); 
        if (loc.where != L_REG) {
            i_mov = I_LEA; 
        }
    }
    //nnc_loc dst = nnc_store(&quad->arg1);
    //if (quad->arg2.kind == ADDR_SCONST) {
    //
    //}
    //nnc_loc src = nnc_store_global(&quad->arg1);
    assert(!nnc_struct_or_union_type(quad->res.type));
    //todo: handle case with memory to memory copy
    gen_instr(i_mov);
    if (quad->arg1.kind == ADDR_ICONST) {
        nnc_gen_operand(&quad->res, &u64_type);
        gen_comma();
        nnc_gen_operand(&quad->arg1, quad->arg1.type);
    }
    else {
        nnc_gen_operand(&quad->res, quad->res.type);
        gen_comma();
        nnc_gen_operand(&quad->arg1, quad->res.type);
    }
}

nnc_static void nnc_gen_label_decl(const nnc_3a_quad* quad) {
    assert(quad->label != 0);
    gen_t0(NNC_LABEL_PREFIX"%u:", quad->label);
}

nnc_static void nnc_gen_op_ref(const nnc_3a_quad* quad) {
    gen_instr(I_LEA);
    nnc_gen_operand(&quad->res, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg1, quad->res.type);
}

nnc_static void nnc_gen_op_deref_copy(const nnc_3a_quad* quad) {
    gen_instr(I_MOV);
    nnc_loc loc = nnc_store(&quad->res, false);
    //todo: refactor this
    if (loc.where == L_REG) {
        gen_t0("%s ptr [", nnc_get_ptr_size(quad->res.type));
        nnc_gen_loc(&loc, &u64_type);
        gen_t0("]");
    }
    else {
        nnc_gen_loc(&loc, quad->res.type);
    }
    gen_comma();
    nnc_gen_operand(&quad->arg1, quad->res.type);
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
        nnc_gen_generic_mov(&quad->res, &quad->arg1);
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
        nnc_gen_generic_mov(&quad->res, &quad->arg1);
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
nnc_static void nnc_gen_one_op_mul(const nnc_3a_quad* quad,
    nnc_asm_instr mul, nnc_bool res_in_rdx) {
    nnc_bool rax_pushed = false;
    nnc_bool rdx_pushed = false;
    nnc_bool arg1_inside_rax = false;
    nnc_bool arg2_inside_rdx = false;
    // checks if first argument of multiplication inside RDX
    arg2_inside_rdx = nnc_addr_inside_reg(&quad->arg1, R_RDX);
    if (!arg2_inside_rdx) {
        // if RDX is currently used, it will be pushed to the stack
        rdx_pushed = nnc_gen_reserve_reg(R_RDX);
    }
    // checks if first argument of multiplication inside RAX
    arg1_inside_rax = nnc_addr_inside_reg(&quad->arg1, R_RAX);
    if (!arg1_inside_rax) {
        // if not, allocate it.
        // if RAX is currently used, it will be pushed to the stack
        rax_pushed = nnc_gen_reserve_reg(R_RAX);
        // copy first argument to RAX
        nnc_gen_operand_to_reg_mov(R_RAX, &quad->arg1);
    }
    gen_instr(mul);
    nnc_gen_operand(&quad->arg2, quad->res.type);
    gen_crlf();
    if (res_in_rdx) {
        if (!arg2_inside_rdx) {
            // copy result of multiplication from RDX
            nnc_gen_mov_reg_to_operand(R_RDX, &quad->arg1, quad->res.type);
        }
    }
    else {
        if (!arg1_inside_rax) {
            // copy result of multiplication from RAX
            nnc_gen_mov_reg_to_operand(R_RAX, &quad->arg1, quad->res.type);
        }
    }
    if (rax_pushed) {
        // restore RAX if it was pushed
        nnc_gen_unreserve_reg(R_RAX);
    }
    if (rdx_pushed) {
        nnc_gen_unreserve_reg(R_RDX);
    }
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
    nnc_gen_3a_addr(NULL, &quad->arg2);
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
nnc_static void nnc_gen_instr_mul(const nnc_3a_quad* quad) {
    nnc_gen_one_op_mul(quad, I_MUL, false);
}

// https://www.felixcloutier.com/x86/imul
nnc_static void nnc_gen_instr_imul(const nnc_3a_quad* quad) {
    if (nnc_is_three_op_mul(quad)) {
        nnc_gen_three_op_mul(quad, I_IMUL);
    }
    else if (nnc_is_two_op_mul(quad)) {
        nnc_gen_two_op_mul(quad, I_IMUL);
    }
    else {
        nnc_gen_one_op_mul(quad, I_IMUL, false);
    }
}

nnc_static void nnc_gen_op_mul(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->res.type));
    assert(
        quad->arg1.kind == ADDR_CGT ||
        quad->arg2.kind == ADDR_CGT
    );
    nnc_asm_instr i_mul = I_MUL;
    if (nnc_signed_type(quad->res.type)) {
        i_mul = I_IMUL;
    }
    switch (i_mul) {
        case I_MUL:  nnc_gen_instr_mul(quad);  break;
        case I_IMUL: nnc_gen_instr_imul(quad); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op_mul: unsupported mul instruction.\n");
        }
    }
}

// https://www.felixcloutier.com/x86/div
nnc_static void nnc_gen_instr_div(const nnc_3a_quad* quad, nnc_bool is_mod_instr) {
    nnc_gen_one_op_mul(quad, I_DIV, is_mod_instr);
}

// https://www.felixcloutier.com/x86/idiv
nnc_static void nnc_gen_instr_idiv(const nnc_3a_quad* quad, nnc_bool is_mod_instr) {
    nnc_gen_one_op_mul(quad, I_IDIV, is_mod_instr);
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
        case I_DIV:  nnc_gen_instr_div(quad, false);  break;
        case I_IDIV: nnc_gen_instr_idiv(quad, false); break;
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
        case I_DIV:  nnc_gen_instr_div(quad, true);  break;
        case I_IDIV: nnc_gen_instr_idiv(quad, true); break;
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
        nnc_gen_operand_to_reg_mov(R_RCX, &quad->arg2);
    }
    // generate shift instruction
    gen_instr(shift);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_t0(", cl");
    // restore RCX if it was pushed
    if (rcx_pushed) {
        gen_crlf();
        nnc_gen_unreserve_reg(R_RCX);
    }
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
    nnc_bool need_to_push = false;
    nnc_loc src = *nnc_get_loc(&quad->arg1);
    nnc_loc dst = nnc_store_arg(&quad->arg1, &need_to_push);
    gen_instr(I_MOV);
    assert(
        dst.where == L_REG || 
        dst.where == L_PARAM_STACK
    );
    if (dst.where == L_REG) {
       // assert(!need_to_push);
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
    gen_t1(NNC_RET_LABEL_PREFIX"%s", glob_current_unit->name);
}

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retf(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->arg1.type));
    assert(quad->arg1.kind == ADDR_CGT);
    nnc_gen_operand_to_reg_mov(R_RAX, &quad->arg1);
    gen_instr(I_JMP);
    gen_t0(NNC_RET_LABEL_PREFIX"%s", glob_current_unit->name);
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

nnc_static void nnc_gen_op_pcall(const nnc_3a_quad* quad) {
    nnc_gen_caller_prepare();
    gen_instr(I_CALL);
    gen_t0("_%s", quad->arg1.exact.name.name);
    gen_crlf();
    nnc_gen_caller_restore();
}

nnc_static void nnc_gen_op_fcall(const nnc_3a_quad* quad) {
    assert(!nnc_real_type(quad->res.type));
    nnc_gen_caller_prepare();
    gen_instr(I_CALL);
    gen_t0("_%s\n", quad->arg1.exact.name.name);
    if (!nnc_addr_inside_reg(&quad->res, R_RAX)) {
        nnc_gen_mov_reg_to_operand(R_RAX,
            &quad->res, quad->res.type);
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
        const nnc_register reg = pushed[i-1];
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
    assert(
        nnc_addr_inside(&quad->arg1, L_REG) &&
        nnc_addr_inside(&quad->arg2, L_REG | LOCATION_MEM)
    );
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
        nnc_gen_operand(&quad->arg1, &u64_type);
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

nnc_static void nnc_gen_op_decl_local(const nnc_3a_quad* quad) {
    nnc_store(&quad->res, false);
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
            case OP_PREPARE_CALL: nnc_gen_op_prepare_for_call(quad); break;
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
            /* Misc operators */
            case OP_DECL_LOCAL: nnc_gen_op_decl_local(quad); break;
            default: {
                nnc_abort_no_ctx("nnc_gen_op: unknown operator.\n");
            }
        }
    }
}

extern void nnc_dump_3a_quad(const nnc_3a_quad* quad);

nnc_static void nnc_gen_bootstrap() {
    fprintf(stderr,
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

nnc_static nnc_u32 nnc_unit_params_calc_stack() {
    nnc_u32 stack_usage = 0;
    const vector(nnc_fn_param*) params = glob_current_unit->params;
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

nnc_static void nnc_store_params(nnc_3a_proc* unit) {
    glob_current_unit->stack_usage = nnc_unit_params_calc_stack();
    const vector(nnc_fn_param*) params = glob_current_unit->params;
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
    nnc_gen_proc_prologue(glob_current_unit);
}

nnc_static void nnc_gen_callee_restore() {
    gen_t0(NNC_RET_LABEL_PREFIX"%s:\n", glob_current_unit->name);
    nnc_gen_proc_epilogue(glob_current_unit);
    gen_t1("ret\n\n");
}

#define GEN_DEBUG 0

#if 0 || GEN_DEBUG == 0
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

extern vector(nnc_3a_lr*) glob_reg_lr[21];

nnc_static nnc_assembly_proc nnc_gen_proc(nnc_3a_proc* proc_3a) {
    glob_current_unit = proc_3a;
    glob_current_asm_proc = (nnc_assembly_proc){
        .code = proc_3a,
        .impl = (nnc_blob_buf){0}
    };
    nnc_blob_buf_init(&glob_current_asm_proc.impl);
    // clear register live ranges from previous procudure
    for (nnc_u64 i = 0; i < nnc_arr_size(glob_reg_lr); i++) {
        buf_free(glob_reg_lr[i]);
    }
    memset(glob_reg_lr, 0, sizeof(glob_reg_lr));
    // store params
    nnc_store_params(proc_3a);
    for (nnc_u64 i = 0; i < buf_len(proc_3a->quads); i++) {
        const nnc_3a_quad* quad = &proc_3a->quads[i];
        #if GEN_DEBUG == 1
            gen_t0("# ");
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