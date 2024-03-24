#include "nnc_gen.h"

nnc_static nnc_3a_unit* nnc_glob_unit = NULL;
nnc_u32 nnc_glob_stack_offset = 8;
nnc_u32 nnc_glob_param_stack_offset = 0;

#define gen_t0(...)  fprintf(stderr, __VA_ARGS__)
#define gen_t1(...)  fprintf(stderr, "   " __VA_ARGS__)
#define gen_instr(i) gen_t1("%s ", nnc_asm_instr_str[i]);
#define gen_comma()  gen_t0(", ")
#define gen_crlf()   fprintf(stderr, "\r\n")

nnc_static const char* nnc_get_ptr_size(const nnc_type* type) {
    switch (type->size) {
        case 1: return "byte";
        case 2: return "word";
        case 4: return "dword";
        case 8: return "qword";
        default: nnc_abort_no_ctx("nnc_get_ptr_size: unsupported size.\n");
    }
    return NULL;
}

nnc_static void nnc_gen_storage(const nnc_3a_storage* at, const nnc_type* type) {
    switch (at->where) {
        case STORAGE_REG: {
            gen_t0("%s", nnc_asm_reg_str[at->reg]);
            break;
        }
        case STORAGE_STACK_SEG: {
            //const char* ptr_size = nnc_get_ptr_size(type);
            //gen_t0("%s ptr [rbp-%u]", ptr_size, at->mem_offset);
            gen_t0("[rbp-%u]", at->mem_offset);
            break;
        }
        case STORAGE_DATA_SEG: {
            nnc_abort_no_ctx("nnc_gen_storage: STORAGE_DATA_SEG unsupported.\n");
            break;
        }
        default: {
            nnc_abort_no_ctx("nnc_gen_storage: unsupported storage type.\n");
        }
    }
}

nnc_static void nnc_get_and_gen_storage(const nnc_3a_addr* addr) {
    const nnc_3a_storage* storage = nnc_store_generic(
        nnc_glob_unit, addr, STORE_LOCAL);
    nnc_gen_storage(storage, addr->type);
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

nnc_static void nnc_gen_addr(const nnc_3a_storage* at, const nnc_3a_addr* addr) {
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

nnc_static nnc_bool nnc_direct_addr(const nnc_3a_addr* addr) {
    return addr->kind == ADDR_NAME   ||
           addr->kind == ADDR_ICONST ||
           addr->kind == ADDR_FCONST; 
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
    nnc_get_and_gen_storage(&quad->res);
    gen_comma();
    if (nnc_direct_addr(&quad->arg1)) {
        nnc_gen_addr(NULL, &quad->arg1);
    }
    else {
        nnc_get_and_gen_storage(&quad->arg1);
    }
}

nnc_static void nnc_gen_op_ref(const nnc_3a_quad* quad) {
    gen_instr(I_LEA);
    nnc_get_and_gen_storage(&quad->res);
    gen_comma();
    if (nnc_direct_addr(&quad->arg1)) {
        nnc_gen_addr(NULL, &quad->arg1);
    }
    else {
        nnc_get_and_gen_storage(&quad->arg1);
    }
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
    if (nnc_direct_addr(&quad->arg1)) {
        nnc_gen_addr(NULL, &quad->arg1);
    }
    else {
        nnc_get_and_gen_storage(&quad->arg1);
    } 
}

nnc_static void nnc_gen_op_minus(const nnc_3a_quad* quad) {
    gen_instr(I_NEG);
    if (nnc_direct_addr(&quad->arg1)) {
        nnc_gen_addr(NULL, &quad->arg1);
    }
    else {
        nnc_get_and_gen_storage(&quad->arg1);
    }
}

nnc_static void nnc_gen_op_add(const nnc_3a_quad* quad) {
    gen_instr(I_ADD);
    if (nnc_direct_addr(&quad->arg1)) {
        nnc_gen_addr(NULL, &quad->arg1);
    }
    else {
        nnc_get_and_gen_storage(&quad->arg1);
    } 
    gen_comma();
    if (nnc_direct_addr(&quad->arg2)) {
        nnc_gen_addr(NULL, &quad->arg2);
    }
    else {
        nnc_get_and_gen_storage(&quad->arg2);
    }
}

nnc_static void nnc_gen_op(const nnc_3a_quad* quad) {
    switch (quad->op) {
        case OP_REF:        nnc_gen_op_ref(quad);  break;
        case OP_COPY:       nnc_gen_op_copy(quad); break;
        case OP_DEREF_COPY: nnc_gen_op_deref_copy(quad); break;
        
        case OP_MINUS:      nnc_gen_op_minus(quad); break;

        case OP_ADD:        nnc_gen_op_add(quad); break;
        default: nnc_abort_no_ctx("nnc_gen_op: unknown operator.\n");
    }
}

extern void nnc_dump_3a_quad(const nnc_3a_quad* quad);

void nnc_gen_unit(nnc_3a_unit* unit) {
    nnc_glob_unit = unit;
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
    }
    gen_t0("# -----CODE-----\n");
    gen_t1("mov rsp, rbp\n");
    gen_t1("pop rbp\n");
    gen_t1("ret\n\n");
}