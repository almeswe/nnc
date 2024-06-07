#ifndef __NNC_x86_64_GEN_H__
#define __NNC_x86_64_GEN_H__

#include "nnc_blob.h"
#include "nnc_misc.h"
#include "nnc_gen_sa.h"
#include "nnc_ast_eval.h"

#define SEG_TEXT (&glob_state.curr.asm_proc.impl)
#define SEG_DATA (&glob_state.glob.asm_file.ds_impl)

#define data_put(...) nnc_blob_buf_putf(SEG_DATA, __VA_ARGS__)
#define text_put(...) nnc_blob_buf_putf(SEG_TEXT, __VA_ARGS__)

/**
 * @brief Represents assembly procedure
 *  that is currently being generating.
 */
typedef struct _nnc_asm_proc {
    nnc_3a_proc* code; // Pointer to original ir procedure.
    nnc_blob_buf impl; // Buffer which contains assembly implementation.
} nnc_asm_proc;

/**
 * @brief Represents assembly generated file. 
 */
typedef struct _nnc_asm_file {
    nnc_blob_buf ds_impl;       // Buffer which contains data segment implementation.
    nnc_blob_buf ts_impl;       // Buffer which contains text segment implementation.
    vector(nnc_asm_proc) procs; // Vector of implemented procedures.
} nnc_asm_file;

/**
 * @brief Generates assembly for global ir symbol 
 *  (procedure or global variable for now).
 * @param ir Global ir symbol to be generated. 
 * @return Returns assembly file representation 
 *  which contains generated ir symbol. 
 */
nnc_asm_file* nnc_gen(
    nnc_ir_sym ir_sym
);

nnc_blob_buf nnc_build(
    nnc_asm_file* file
);

#endif