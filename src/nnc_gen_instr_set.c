#include "nnc_gen_instr_set.h"

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
    [I_SAL]       = "sal",
    [I_SAR]       = "sar",
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