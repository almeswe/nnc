#ifndef __NNC_GLOBAL_STATE_H__
#define __NNC_GLOBAL_STATE_H__

#include "nnc_core.h"

#define glob_argv     glob_state.glob.argv
#define glob_ir       glob_state.glob.ir
#define glob_ast      glob_state.curr.ast
#define glob_compiled glob_state.glob.compiled
#define glob_asm_file glob_state.glob.asm_file
#define glob_asm_proc glob_state.curr.asm_proc.code
#define glob_canarie  glob_state.glob.canarie
#define glob_arena    glob_state.glob.arena
#define glob_exception_stack glob_state.glob.exc_stack

/**
 * @brief Global state of the compiler. 
 */
typedef struct _nnc_cstate {
    struct _nnc_glob_state {
        nnc_argv argv;
        nnc_arena arena;
        nnc_error_canarie canarie;
        nnc_exception_stack exc_stack;
        nnc_asm_file asm_file;
        vector(nnc_ir_sym) ir;
        vector(nnc_blob_buf) compiled;
    } glob;
    struct _nnc_curr_state {
        nnc_ast* ast;
        nnc_ir_sym* ir_sym;
        nnc_asm_proc asm_proc;
    } curr;
} nnc_cstate;

extern nnc_cstate glob_state;

void nnc_reset_glob_state();
void nnc_reset_curr_state();

#endif