#ifndef __NNC_x86_64_ISA_H__
#define __NNC_x86_64_ISA_H__

typedef char* nnc_asm_inst;

#define I_ADD    "add"
#define I_AND    "and"
#define I_CALL   "call"
#define I_CMP    "cmp"
#define I_CWD    "cwd"
#define I_CDQ    "cdq"
#define I_CQO    "cqo"
#define I_DEC    "dec"
#define I_DIV    "div"
#define I_ENTER  "enter"
#define I_HLT    "hlt"
#define I_IDIV   "idiv"
#define I_IMUL   "imul"
#define I_INC    "inc"
#define I_JA     "ja"
#define I_JAE    "jae"
#define I_JB     "jb"
#define I_JBE    "jbe"
#define I_JE     "je"
#define I_JG     "jg"
#define I_JGE    "jge"
#define I_JL     "jl"
#define I_JLE    "jle"
#define I_JNE    "jne"
#define I_JNZ    "jnz"
#define I_JZ     "jz"
#define I_JMP    "jmp"
#define I_LEA    "lea"
#define I_LEAVE  "leave"
#define I_MOV    "mov"
#define I_MOVSX  "movsx"
#define I_MOVZX  "movzx"
#define I_MOVSXD "movsxd"
#define I_MUL    "mul"
#define I_NEG    "neg"
#define I_NOP    "nop"
#define I_NOT    "not"
#define I_OR     "or"
#define I_POP    "pop"
#define I_PUSH   "push"
#define I_RET    "ret"
#define I_SAL    "sal"
#define I_SAR    "sar"
#define I_SHL    "shl"
#define I_SHR    "shr"
#define I_SUB    "sub"
#define I_TEST   "test"
#define I_XOR    "xor"

#define P_BYTE  "byte"
#define P_WORD  "word"
#define P_DWORD "dword"
#define P_QWORD "qword"

#define PS_BYTE  1
#define PS_WORD  2
#define PS_DWORD 4
#define PS_QWORD 8

#define DR_LABEL   ".L"
#define DR_RLABEL  ".Lfini_"

#define DR_GLOBAL  ".globl"
#define DR_GLOBAL2 ".global"

#define DR_BYTE ".byte"
#define DR_LONG ".long"

#define DR_DATA    ".data"
#define DR_TEXT    ".text"
#define DR_ISYNTAX ".intel_syntax"

#endif