#include "nnc_gen.h"

//todo: make sure that each operands matches to available operand for instruction
//todo: add ability to allocate registers for immediates

nnc_static nnc_3a_unit* nnc_glob_unit = NULL;
nnc_static nnc_u32 nnc_glob_units = 0;
nnc_u32 nnc_glob_stack_offset = 8;
nnc_u32 nnc_glob_param_stack_offset = 0;

#define gen_t0(...)  fprintf(stderr, __VA_ARGS__)
#define gen_t1(...)  fprintf(stderr, "   " __VA_ARGS__)
#define gen_note(n)  gen_t0(RESET "# %s\n" GRN, n);
#define gen_instr(i) gen_t1("%s ", nnc_asm_instr_str[i])
#define gen_comma()  gen_t0(", ")
#define gen_crlf()   fprintf(stderr, "\r\n")

#define NNC_LABEL_PREFIX     "label_"
#define NNC_RET_LABEL_PREFIX "label_ret_"

nnc_static nnc_bool nnc_forced_reg_preserve(const nnc_3a_addr* generic, nnc_asm_reg reg) {
    //todo: make change in live range, not just push.
    if (nnc_reg_in_use(nnc_glob_unit, reg)) {
        gen_t1("push %s\n", nnc_asm_reg_str[reg]);
        return true;
    }
    return false;
}

nnc_static void nnc_reg_return(const nnc_3a_addr* generic, nnc_asm_reg reg) {
    //todo: make change in live range, not just push.
    gen_t1("pop %s\n", nnc_asm_reg_str[reg]);
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
    switch (type->size) {
        case 1: return "byte";
        case 2: return "word";
        case 4: return "dword";
        case 8: return "qword";
        default: {
            nnc_abort_no_ctx("nnc_get_ptr_size: unsupported size.\n");
        }
    }
    return NULL;
}

nnc_static const char* nnc_get_reg_of_size(nnc_asm_reg reg, const nnc_type* type) {
    static const char* arr[][4] = {
        [R_RAX]  = { "ah", "ax", "eax", "rax" },
        [R_RBX]  = { "bh", "bx", "ebx", "rbx" },
        [R_RCX]  = { "ch", "cx", "ecx", "rcx" },
        [R_RDX]  = { "dh", "dx", "edx", "rdx" },
        [R_RSI]  = { "sih", "si", "esi", "rsi" },
        [R_RDI]  = { "dih", "di", "edi", "rdi" },
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
    if (nnc_real_type(type)) {
        return arr[reg][0];
    }
    else {
        switch (type->size) {
            case 1: return arr[reg][0];
            case 2: return arr[reg][1];
            case 4: return arr[reg][2];
            case 8: return arr[reg][3];
            default: {
                nnc_abort_no_ctx("nnc_get_reg_of_size: unsupported size.\n");
            }
        }
    }
    return NULL;
}

nnc_static nnc_3a_lr* nnc_get_lr(nnc_3a_unit* unit, const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:  return (nnc_3a_lr*)map_get(unit->lr_cgt, addr->exact.cgt); 
        case ADDR_NAME: return (nnc_3a_lr*)map_get_s(unit->lr_var, addr->exact.name.name);
        default: {
            nnc_abort_no_ctx("bad addr kind\n");
        }
    }
    return NULL;
}

nnc_static const nnc_3a_storage* nnc_get_storage(const nnc_3a_addr* addr) {
    const nnc_3a_lr* lr = nnc_get_lr(nnc_glob_unit, addr);
    return &lr->storage;
}

nnc_static nnc_asm_operand nnc_get_operand_type(const nnc_3a_addr* addr) {
    nnc_asm_operand op_type = 0;
    switch (addr->kind) {
        case ADDR_CGT:
        case ADDR_NAME: {
            const nnc_3a_storage* storage = nnc_get_storage(addr);
            assert(storage != NULL);
            assert(
                storage->where == STORAGE_REG      ||
                storage->where == STORAGE_DATA_SEG || 
                storage->where == STORAGE_STACK_SEG
            );
            switch (storage->where) {
                case STORAGE_REG:        op_type = A_REG8; break;
                case STORAGE_DATA_SEG:   op_type = A_MEM8; break;
                case STORAGE_STACK_SEG:  op_type = A_MEM8; break;
                default: {
                    nnc_abort_no_ctx("nnc_get_operand_type: unknown storage.\n");
                }
            }
            switch (addr->type->size) {
                case 1: return op_type;
                case 2: return op_type << 1;
                case 4: return op_type << 2;
                case 8: return op_type << 3;
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

nnc_static nnc_bool nnc_addr_inside_reg(const nnc_3a_addr* addr, nnc_asm_reg reg) {
    const nnc_3a_storage* storage = nnc_get_storage(addr);
    if (storage->where == STORAGE_REG) {
        return storage->reg == reg;
    }
    return false;
}

nnc_static nnc_bool nnc_addr_inside(const nnc_3a_addr* addr, nnc_3a_storage_type at) {
    const nnc_3a_storage* storage = nnc_get_storage(addr);
    return (storage->where & at) != 0;
}

nnc_static void nnc_gen_storage(const nnc_3a_storage* at, const nnc_type* type) {
    switch (at->where) {
        case STORAGE_REG: {
            gen_t0("%s", nnc_get_reg_of_size(at->reg, type));
            break;
        }
        case STORAGE_STACK_SEG: {
            const char* ptr_size = nnc_get_ptr_size(type);
            gen_t0("%s ptr [rbp-%u]", ptr_size, at->mem_offset);
            //gen_t0("[rbp-%u]", at->mem_offset);
            break;
        }
        case STORAGE_DATA_SEG: {
            nnc_abort_no_ctx("nnc_gen_storage: STORAGE_DATA_SEG unsupported.\n");
        }
        default: {
            nnc_abort_no_ctx("nnc_gen_storage: unsupported storage type.\n");
        }
    }
}

nnc_static void nnc_gen_addr_cgt(const nnc_3a_addr* addr) {
    const nnc_3a_storage* at = nnc_store_generic(
        nnc_glob_unit, addr, STORE_LOCAL);
    nnc_gen_storage(at, addr->type);
}

nnc_static void nnc_gen_addr_var(const nnc_3a_addr* addr) {
    const nnc_3a_storage* at = nnc_store_generic(
        nnc_glob_unit, addr, STORE_LOCAL);
    nnc_gen_storage(at, addr->type);
}

nnc_static void nnc_gen_addr_iconst(const nnc_3a_addr* addr) {
    if (nnc_integral_type(addr->type)) {
        gen_t0("%ld", (nnc_i64)addr->exact.iconst.iconst);
    }
    else {
        gen_t0("%lu", (nnc_u64)addr->exact.iconst.iconst);
    }
}

nnc_static void nnc_gen_addr_fconst(const nnc_3a_addr* addr) {
    nnc_abort_no_ctx("nnc_gen_addr_fconst: is not implemented.\n");    
}

nnc_static void nnc_gen_3a_addr(const nnc_3a_storage* at, const nnc_3a_addr* addr) {
    // in this case check if storage is presented as direct store
    // it means that `nnc_3a_addr` is direct instruction argument 
    if (at != NULL && at->where != STORAGE_NONE) {
        gen_instr(I_MOV);
        nnc_gen_storage(at, addr->type);
        gen_comma();
    }
    switch (addr->kind) {
        case ADDR_CGT:    nnc_gen_addr_cgt(addr);    break;
        case ADDR_NAME:   nnc_gen_addr_var(addr);    break;
        case ADDR_ICONST: nnc_gen_addr_iconst(addr); break;
        case ADDR_FCONST: nnc_gen_addr_fconst(addr); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op: unknown addr kind.\n");
        }
    }
}

nnc_static void nnc_gen_operand(const nnc_3a_addr* addr, const nnc_type* type) {
    const nnc_3a_storage* storage = NULL;
    if (addr->kind == ADDR_CGT || addr->kind == ADDR_NAME) {
        storage = nnc_store_generic(
            nnc_glob_unit, addr, STORE_LOCAL);
        nnc_gen_storage(storage, type);
    }
    else {
        nnc_gen_3a_addr(NULL, addr);                
    }
}

nnc_static void nnc_gen_mov_operand_to_reg(nnc_asm_reg reg, 
    const nnc_3a_addr* addr, const nnc_type* type) {
    gen_instr(I_MOV);
    gen_t0("%s, ", nnc_get_reg_of_size(reg, type));
    nnc_gen_operand(addr, type);
    gen_crlf();
}

nnc_static void nnc_gen_mov_reg_to_operand(nnc_asm_reg reg, 
    const nnc_3a_addr* addr, const nnc_type* type) {
    gen_instr(I_MOV);
    nnc_gen_operand(addr, type);
    gen_comma();
    gen_t0("%s", nnc_get_reg_of_size(reg, type));
    gen_crlf();
}

nnc_static void nnc_gen_op_copy(const nnc_3a_quad* quad) {
    nnc_asm_instr i_mov = I_MOV;
    if (nnc_real_type(quad->res.type)) {
        i_mov = I_MOVSS;
        if (quad->res.type->kind == T_PRIMITIVE_F64) {
            i_mov = I_MOVSD;
        }        
    }
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
    const nnc_3a_storage* storage = nnc_store_generic(
        nnc_glob_unit, &quad->res, STORE_LOCAL);
    //todo: refactor this
    if (storage->where == STORAGE_REG) {
        gen_t0("%s ptr [", nnc_get_ptr_size(quad->res.type));
        nnc_gen_storage(storage, &u64_type);
        gen_t0("]");
    }
    else {
        nnc_gen_storage(storage, quad->res.type);
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
        quad->arg2.kind == ADDR_CGT
    );
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg2, quad->res.type);
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
        rdx_pushed = nnc_forced_reg_preserve(NULL, R_RDX);
    }
    // checks if first argument of multiplication inside RAX
    arg1_inside_rax = nnc_addr_inside_reg(&quad->arg1, R_RAX);
    if (!arg1_inside_rax) {
        // if not, allocate it.
        // if RAX is currently used, it will be pushed to the stack
        rax_pushed = nnc_forced_reg_preserve(&quad->arg1, R_RAX);
        // copy first argument to RAX
        nnc_gen_mov_operand_to_reg(R_RAX, &quad->arg1, quad->res.type);
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
        nnc_reg_return(&quad->arg1, R_RAX);
    }
    if (rdx_pushed) {
        nnc_reg_return(NULL, R_RDX);
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
    //todo: test this
    gen_instr(mul);
    nnc_gen_operand(&quad->arg1, quad->res.type);
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
    const nnc_3a_storage* arg1_storage = nnc_get_storage(&quad->arg1);
    const nnc_3a_storage* arg2_storage = nnc_get_storage(&quad->arg2);
    if (arg1_storage->where == STORAGE_REG) {
        if (arg2_storage->where == STORAGE_REG       ||
            arg2_storage->where == STORAGE_DATA_SEG  ||
            arg2_storage->where == STORAGE_STACK_SEG) {
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
    const nnc_3a_storage* arg1_storage = nnc_get_storage(&quad->arg1);
    if (arg1_storage->where == STORAGE_REG) {
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
    if (nnc_is_two_op_mul(quad)) {
        nnc_gen_two_op_mul(quad, I_IMUL);
    }
    else if (nnc_is_three_op_mul(quad)) {
        nnc_gen_three_op_mul(quad, I_IMUL);
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
        rcx_pushed = nnc_forced_reg_preserve(&quad->arg2, R_RCX);
        // copy second argument (count of shifts) to RCX 
        nnc_gen_mov_operand_to_reg(R_RCX, &quad->arg2, quad->arg2.type);
    }
    // generate shift instruction
    gen_instr(shift);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_t0(", cl");
    // restore RCX if it was pushed
    if (rcx_pushed) {
        gen_crlf();
        nnc_reg_return(&quad->arg2, R_RCX);
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

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retp(const nnc_3a_quad* quad) {
    gen_instr(I_JMP);
    gen_t1(NNC_RET_LABEL_PREFIX"%u", nnc_glob_units);
}

// https://www.felixcloutier.com/x86/ret
nnc_static void nnc_gen_op_retf(const nnc_3a_quad* quad) {
    //todo: float type extension
    assert(!nnc_real_type(quad->arg1.type));
    assert(quad->arg1.kind == ADDR_CGT);
    //if (!nnc_addr_inside_reg(&quad->arg1, R_RAX)) {
    //    gen_t1("mov %s", nnc_get_reg_of_size(R_RAX, quad->arg1.type));
    //    nnc_gen_operand(&quad->arg1, quad->arg1.type);     
    //    gen_crlf();
    //}
    gen_instr(I_JMP);
    gen_t0(NNC_RET_LABEL_PREFIX"%s", nnc_glob_unit->name);
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
        nnc_addr_inside(&quad->arg1, STORAGE_REG) &&
        nnc_addr_inside(&quad->arg2, STORAGE_REG | STORAGE_MEM)
    );
    gen_instr(I_CMP);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg2, quad->res.type);
    gen_crlf();
}

nnc_static void nnc_gen_instr_cmp_x(const nnc_3a_quad* quad, nnc_u32 with_x) {
    assert(
        quad->arg1.kind == ADDR_CGT || 
        nnc_addr_inside(&quad->arg1, STORAGE_REG | STORAGE_MEM)
    );
    gen_instr(I_CMP);
    if (nnc_addr_inside(&quad->arg1, STORAGE_REG)) {
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

nnc_static void nnc_gen_op(const nnc_3a_quad* quad) {
    if (quad->label != 0) {
        nnc_gen_label_decl(quad);
    }
    else {
        switch (quad->op) {
            /* Other operators */
            case OP_REF:        nnc_gen_op_ref(quad);  break;
            case OP_COPY:       nnc_gen_op_copy(quad); break;
            case OP_DEREF_COPY: nnc_gen_op_deref_copy(quad); break;
            /* Unary operators */
            case OP_PLUS:   nnc_gen_op_plus(quad);  break;
            case OP_MINUS:  nnc_gen_op_minus(quad); break;
            case OP_BW_NOT: nnc_gen_op_not(quad);   break;
            /* Binary operators */
            case OP_ADD: nnc_gen_op_add(quad); break;
            case OP_SUB: nnc_gen_op_sub(quad); break;
            case OP_MUL: nnc_gen_op_mul(quad); break;
            case OP_DIV: nnc_gen_op_div(quad); break;
            case OP_MOD: nnc_gen_op_mod(quad); break;
            case OP_SHR: nnc_gen_op_shr(quad); break;
            case OP_SHL: nnc_gen_op_shl(quad); break;
            case OP_SAL: nnc_gen_op_sal(quad); break;
            case OP_SAR: nnc_gen_op_sar(quad); break;
            case OP_BW_OR:  nnc_gen_op_or(quad);  break;
            case OP_BW_AND: nnc_gen_op_and(quad); break;
            case OP_BW_XOR: nnc_gen_op_xor(quad); break;
            /* Other operators */
            case OP_RETP: nnc_gen_op_retp(quad); break;
            case OP_RETF: nnc_gen_op_retf(quad); break;
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
    "_exit:\n"
    "   mov rdi, rax\n"
    "   mov rax, 60\n"
    "   syscall\n"
    "\n"
    "_start:\n"
    "   call _main\n"
    "   call _exit\n"
    "   ret\n\n"
    );
}

#if 1
    #undef RED
    #undef GRN
    #undef RESET
    #define RED ""
    #define GRN ""
    #define RESET "" 
#endif

void nnc_gen_unit(nnc_3a_unit* unit) {
    nnc_glob_unit = unit;
    nnc_glob_units++;
    nnc_glob_unit->quad_pointer = 0;
    gen_t0("_%s:\n", unit->name);
    gen_t1("push rbp\n");
    gen_t1("mov rbp, rsp\n");
    //todo: allocate exact amount of memory
    gen_t1("sub rsp, 32\n");
    gen_t0("# -----CODE-----\n");
    for (nnc_u64 i = 0; i < buf_len(unit->quads); i++) {
        //gen_t0("# ");
        fprintf(stderr, RED);
        //nnc_dump_3a_quad(&unit->quads[i]);
        fprintf(stderr, GRN);
        nnc_gen_op(&unit->quads[i]);
        fprintf(stderr, RESET);
        gen_crlf();
        nnc_glob_unit->quad_pointer++;
    }
    gen_t0("# -----CODE-----\n");
    gen_t0(NNC_RET_LABEL_PREFIX"%s:\n", nnc_glob_unit->name);
    gen_t1("mov rsp, rbp\n");
    gen_t1("pop rbp\n");
    gen_t1("ret\n\n");
}

void nnc_gen_code(_vec_(nnc_3a_unit) code) {
    nnc_gen_bootstrap();
    for (nnc_u64 i = 0; i < buf_len(code); i++) {
        nnc_gen_unit(&code[i]);
    }
}