#ifndef __NNC_x86_64_GEN_INSTR_SET_H__
#define __NNC_x86_64_GEN_INSTR_SET_H__

#include "nnc_arena.h"

typedef enum _nnc_x86_64_asm_instr {
    /* OP_NONE */
    I_NOP, 
    /* OP_ADD */
    I_ADD,
    I_ADDSS,
    I_ADDSD,
    /* OP_SUB */
    I_SUB,
    I_SUBSS,
    I_SUBSD,
    /* OP_MUL */
    I_MUL,
    I_IMUL,
    I_MULSS,
    I_MULSD,
    /* OP_DIV */
    /* OP_MOD */
    I_DIV,
    I_IDIV,
    I_DIVSS,
    I_DIVSD,
    /* OP_SHR */
    I_SHR,
    /* OP_SHL */
    I_SHL,
    /* OP_SAL */
    I_SAL,
    /* OP_SAR */
    I_SAR,
    /* OP_BW_OR */
    I_OR,
    /* OP_BW_AND */
    I_AND,
    /* OP_BW_XOR */
    I_XOR,
    /* OP_PLUS - skip */
    /* OP_MINUS */
    I_NEG,
    /* OP_BW_NOT */
    I_NOT,
    /* OP_UJUMP */
    I_JMP,
    /* OP_CJUMPT */
    I_JNZ,
    /* OP_CJUMPF */
    I_JZ,
    /* OP_CJUMPLT */
    I_JL,
    I_JB,
    /* OP_CJUMPGT */
    I_JG,
    I_JA,
    /* OP_CJUMPLTE */
    I_JLE,
    I_JBE,
    /* OP_CJUMPGTE */
    I_JGE,
    I_JAE,
    /* OP_CJUMPE */
    I_JE,
    /* OP_CJUMPNE */
    I_JNE,
    /* OP_COPY */
    I_MOV,
    I_MOVSX,
    I_MOVZX,
    I_MOVSS,
    I_MOVSD,
    I_MOVSXD,
    /* OP_CAST */
    I_CVTSD2SI,    /* convert scalar double-precision floating-point values to a doubleword integer */
    I_CVTSD2SS,    /* convert scalar double-precision floating-point values to scalar single-precision floating-point values */
    I_CVTSS2SD,    /* convert scalar single-precision floating-point values to scalar double-precision floating-point values */
    I_CVTSI2SS,    /* convert doubleword integer to scalar single-precision floating-point value  */
    I_CVTTSS2SI,   /* convert with truncation scalar single-precision floating-point value to scalar doubleword integer */
    /* OP_ARG - skip */
    /* OP_PCALL */
    /* OP_FCALL */
    I_CALL,
    /* OP_RETP */
    /* OP_RETF */
    I_RET,
    /* OP_REF */
    I_LEA,
    /* OP_DEREF      - skip, brackets used in asm */
    /* OP_DEREF_COPY - skip, brackets used in asm */
    /* Additional instructions */
    I_CMP,
    I_CMPSD,
    I_CMPSS,
    I_PUSH,
    I_POP,
} nnc_asm_instr;

#define A_ANY       (A_ANY_MEM | A_ANY_REG | A_ANY_IMM)
#define A_NOT_IMM   (A_ANY_MEM | A_ANY_REG)
#define A_ANY_MEM   (A_MEM8 | A_MEM16 | A_MEM32 | A_MEM64)
#define A_ANY_REG   (A_REG8 | A_REG16 | A_REG32 | A_REG64)
#define A_ANY_IMM   (A_IMM8 | A_IMM16 | A_IMM32 | A_IMM64)
#define A_NOT_IMM64 (A_IMM8 | A_IMM16 | A_IMM32)

#define A_ANY_LOCATABLE     (A_ANY_REG | A_ANY_MEM)
#define A_ANY_BUT_NOT_IMM64 (A_ANY_REG | A_ANY_MEM | A_NOT_IMM64)

typedef enum _nnc_x86_64_asm_operand {
    A_IMM8  = 0b000000000001,
    A_IMM16 = 0b000000000010,
    A_IMM32 = 0b000000000100,
    A_IMM64 = 0b000000001000,
    A_MEM8  = 0b000000010000,
    A_MEM16 = 0b000000100000,
    A_MEM32 = 0b000001000000,
    A_MEM64 = 0b000010000000,
    A_REG8  = 0b000100000000,
    A_REG16 = 0b001000000000,
    A_REG32 = 0b010000000000,
    A_REG64 = 0b100000000000
} nnc_asm_operand;

extern const char* nnc_asm_instr_str[];

#endif