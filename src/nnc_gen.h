#ifndef __NNC_x86_64_GEN_H__
#define __NNC_x86_64_GEN_H__

#include "nnc_3a.h"
#include "nnc_blob.h"
#include "nnc_misc.h"
#include "nnc_ast_eval.h"
#include "nnc_gen_instr_set.h"
#include "nnc_gen_reg_alloc.h"

#define SEG_TEXT (&glob_current_asm_proc.impl)
#define SEG_DATA (&glob_current_asm_file->data_segment_impl)

#define data_put(...) nnc_blob_buf_putf(SEG_DATA, __VA_ARGS__)
#define text_put(...) nnc_blob_buf_putf(SEG_TEXT, __VA_ARGS__)

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
extern nnc_ast* glob_current_ast;

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