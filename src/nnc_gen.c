#include "nnc_gen.h"

//todo: make sure that each operands matches to available operand for instruction
//todo: add ability to allocate registers for immediates

nnc_static nnc_3a_unit* nnc_glob_unit = NULL;
nnc_u32 nnc_glob_stack_offset = 8;
nnc_u32 nnc_glob_param_stack_offset = 0;

#define gen_t0(...)  fprintf(stderr, __VA_ARGS__)
#define gen_t1(...)  fprintf(stderr, "   " __VA_ARGS__)
#define gen_note(n)  gen_t0(RESET "# %s\n" GRN, n);
#define gen_instr(i) gen_t1("%s ", nnc_asm_instr_str[i])
#define gen_comma()  gen_t0(", ")
#define gen_crlf()   fprintf(stderr, "\r\n")

typedef enum _nnc_instr_operand {
    A_IMM = 0b001, A_MEM = 0b010, A_REG = 0b100
} nnc_instr_operand;

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
nnc_static nnc_bool nnc_able_to_inline_addr(const nnc_3a_addr* addr, nnc_u8 allowed) {
    switch (addr->kind) {
        case ADDR_FCONST:
        case ADDR_ICONST: {
            return (allowed & A_IMM) != 0;
        }
        case ADDR_NAME: {
            return (allowed & A_MEM) != 0;
        }
        default: return false;
    }
}

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

nnc_static nnc_bool nnc_3a_addr_in_reg(const nnc_3a_addr* addr, nnc_asm_reg reg) {
    const nnc_3a_storage* storage = nnc_get_storage(addr);
    if (storage->where == STORAGE_REG) {
        return storage->reg == reg;
    }
    return false;
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
    gen_instr(i_mov);
    nnc_gen_operand(&quad->res, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg1, quad->res.type);
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
    if (storage->where == STORAGE_REG) {
        gen_t0("[");
    }
    nnc_gen_storage(storage, quad->res.type);
    if (storage->where == STORAGE_REG) {
        gen_t0("]");
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
 * @return 
 */
nnc_static void nnc_gen_one_op_mul(const nnc_3a_quad* quad, nnc_asm_instr mul) {
    nnc_bool rax_pushed = false;
    nnc_bool rax_contains_arg = false;
    // checks if first argument of multiplication inside RAX
    rax_contains_arg = nnc_3a_addr_in_reg(&quad->arg1, R_RAX);
    if (!rax_contains_arg) {
        // if not, allocate it.
        // if RAX is currently used, it will be pushed to the stack
        rax_pushed = nnc_forced_reg_preserve(&quad->arg1, R_RAX);
        // copy first argument to RAX
        nnc_gen_mov_operand_to_reg(R_RAX, &quad->arg1, quad->res.type);
    }
    gen_instr(mul);
    nnc_gen_operand(&quad->arg2, quad->res.type);
    gen_crlf();
    if (!rax_contains_arg) {
        // copy result of multiplication from RAX
        nnc_gen_mov_reg_to_operand(R_RAX, &quad->arg1, quad->res.type);
    }
    if (rax_pushed) {
        // restore RAX if it was pushed
        nnc_reg_return(&quad->arg1, R_RAX);
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
    gen_instr(mul);
    nnc_gen_operand(&quad->arg1, quad->res.type);
    gen_comma();
    nnc_gen_operand(&quad->arg2, quad->res.type);
}

/**
 * @brief Check if multiplication is two operand mode.
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

// https://www.felixcloutier.com/x86/mul
nnc_static void nnc_gen_instr_mul(const nnc_3a_quad* quad) {
    nnc_gen_one_op_mul(quad, I_MUL);
}

// https://www.felixcloutier.com/x86/imul
nnc_static void nnc_gen_instr_imul(const nnc_3a_quad* quad) {
    if (nnc_is_two_op_mul(quad)) {
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
        case I_MUL:  nnc_gen_instr_mul(quad);  break;
        case I_IMUL: nnc_gen_instr_imul(quad); break;
        default: {
            nnc_abort_no_ctx("nnc_gen_op_mul: unsupported mul instruction.\n");
        }
    }
    //if (nnc_get_storage(&quad->arg1)->where != STORAGE_REG) {
    //    goto one_operand_form;
    //}
}

nnc_static void nnc_gen_op(const nnc_3a_quad* quad) {
    switch (quad->op) {
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
        default: {
            nnc_abort_no_ctx("nnc_gen_op: unknown operator.\n");
        }
    }
}

extern void nnc_dump_3a_quad(const nnc_3a_quad* quad);

void nnc_gen_unit(nnc_3a_unit* unit) {
    nnc_glob_unit = unit;
    nnc_glob_unit->quad_pointer = 0;
    gen_t0("_%s:\n", unit->name);
    gen_t1("push rbp\n");
    gen_t1("mov rbp, rsp\n");
    gen_t1("sub rsp, 32\n");
    gen_t0("# -----CODE-----\n");
    for (nnc_u64 i = 0; i < buf_len(unit->quads); i++) {
        gen_t0("# ");
        fprintf(stderr, RED);
        nnc_dump_3a_quad(&unit->quads[i]);
        fprintf(stderr, GRN);
        nnc_gen_op(&unit->quads[i]);
        fprintf(stderr, RESET);
        gen_crlf();
        nnc_glob_unit->quad_pointer++;
    }
    gen_t0("# -----CODE-----\n");
    gen_t1("mov rsp, rbp\n");
    gen_t1("pop rbp\n");
    gen_t1("ret\n\n");
}