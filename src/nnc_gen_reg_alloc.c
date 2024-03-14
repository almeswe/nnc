#include "nnc_gen.h"

const char* nnc_asm_reg_str[] = {
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

const char* nnc_asm_instr_str[] = {
    [I_NOP]       = "nop", 
    [I_ADD]       = "add", 
    [I_ADDSS]     = "addss", 
    [I_ADDSD]     = "addsd",
    [I_SUB]       = "sub",
    [I_SUBSS]     = "subss",
    [I_SUBSD]     = "subsd",
    [I_MUL]       = "mul",
    [I_IMUL]      = "imul",
    [I_MULSS]     = "mulss",
    [I_MULSD]     = "mulsd",
    [I_DIV]       = "div",
    [I_IDIV]      = "idiv",
    [I_DIVSS]     = "divss",
    [I_DIVSD]     = "divsd",
    [I_SHR]       = "shr",
    [I_SHL]       = "shl",
    [I_OR]        = "or",
    [I_AND]       = "and",
    [I_XOR]       = "xor",
    [I_NEG]       = "neg",
    [I_NOT]       = "not",
    [I_JMP]       = "jmp",
    [I_JNZ]       = "jnz",
    [I_JZ]        = "jz",
    [I_JL]        = "jl",
    [I_JB]        = "jb",
    [I_JG]        = "jg",
    [I_JA]        = "ja",
    [I_JLE]       = "jle",
    [I_JBE]       = "jbe",
    [I_JGE]       = "jge",
    [I_JAE]       = "jae",
    [I_JE]        = "je",
    [I_JNE]       = "jne",
    [I_MOV]       = "mov",
    [I_MOVSX]     = "movsx",
    [I_MOVZX]     = "movzx",
    [I_MOVSS]     = "movss",
    [I_MOVSD]     = "movsd",
    [I_CVTSD2SI]  = "cvtsd2si",   
    [I_CVTSD2SS]  = "cvtsd2ss",   
    [I_CVTSS2SD]  = "cvtss2sd",   
    [I_CVTSI2SS]  = "cvtsi2ss",   
    [I_CVTTSS2SI] = "cvttss2si",   
    [I_CALL]      = "call",
    [I_RET]       = "ret",
    [I_LEA]       = "lea",
    [I_CMP]       = "cmp",
    [I_CMPSD]     = "cmpsd",
    [I_CMPSS]     = "cmpss",
    [I_PUSH]      = "push",
    [I_POP]       = "pop"
};

nnc_u8 nnc_asm_regs[nnc_arr_size(nnc_asm_reg_str)] = {0};

nnc_asm_reg nnc_alloc_reg(nnc_bool simd) {
    static nnc_asm_reg p1[] = {
        R_RAX, R_RBX, R_R10, R_R11, R_R12, 
        R_R13, R_R14, R_R15, R_R9,  R_R8, 
        R_RCX, R_RDX, R_RDI, R_RSI 
    };
    static nnc_asm_reg p2[] = {
        R_XMM0, R_XMM1, R_XMM2, R_XMM3,
        R_XMM4, R_XMM5, R_XMM6, R_XMM7
    };
    nnc_asm_reg* p = simd ? p2 : p1;
    size_t p_size = nnc_arr_size(p1);
    if (simd) {
        p_size = nnc_arr_size(p2);
    }
    for (size_t loop = 0;; loop++) {
        for (size_t i = 0; i < p_size; i++) {
            nnc_asm_reg_state* state = &nnc_asm_regs[p[i]];
            switch (*state) {
                case 0: return (*state)++, p[i];
                default: {
                    if (loop > 0) {
                        nnc_abort_no_ctx("register must be preserved.");
                    }
                }
            }
        }
    }
}