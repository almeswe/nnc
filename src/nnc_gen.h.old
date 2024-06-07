#ifndef __NNC_x86_64_GEN_H__
#define __NNC_x86_64_GEN_H__

#include "nnc_3a.h"
#include "nnc_blob.h"
#include "nnc_misc.h"
#include "nnc_ast_eval.h"
#include "nnc_gen_sa.h"

#define SEG_TEXT (&glob_current_asm_proc.impl)
#define SEG_DATA (&glob_current_asm_file->data_segment_impl)

#define data_put(...) nnc_blob_buf_putf(SEG_DATA, __VA_ARGS__)
#define text_put(...) nnc_blob_buf_putf(SEG_TEXT, __VA_ARGS__)

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

extern const char* nnc_asm_instr_str[];

typedef struct _nnc_assembly_proc {
    nnc_3a_proc* code;
    nnc_blob_buf impl;
} nnc_assembly_proc;

typedef struct _nnc_assembly_file {
    nnc_blob_buf data_segment_impl;
    vector(nnc_assembly_proc) procs;
    nnc_bool entry_here;
} nnc_assembly_file;

typedef struct _nnc_executable {
    nnc_u32 linker_opts;
    vector(nnc_assembly_file) files;
} nnc_executable;

extern nnc_3a_proc* glob_current_proc;
extern nnc_assembly_file* glob_current_asm_file;
extern nnc_assembly_proc glob_current_asm_proc;
extern nnc_ast* glob_ast;

nnc_assembly_file* nnc_gen(
    nnc_ir_glob_sym* ir
);

void nnc_gen_stmt(
    nnc_statement* stmt
);

nnc_blob_buf nnc_build(
    nnc_assembly_file file
);

#endif