#ifndef _NNC_TRY_CATCH_STACK_H
#define _NNC_TRY_CATCH_STACK_H

#include <setjmp.h>
#include "nnc_error.h"

#define NNC_EXCEPTION_MAX_DEPTH 16

#define NNC_EXC_DEPTH glob_exception_stack.depth
#define NNC_EXC_STACK glob_exception_stack.stack

#define TRY                 if (nnc_try(setjmp(NNC_EXC_STACK[NNC_EXC_DEPTH])))
#define ETRY                nnc_try_free()
#define THROW(exception)    nnc_throw(exception)
#define CATCH(exception)    else if (nnc_catch(exception))
#define FINALLY

typedef struct _nnc_exception_stack {
    nnc_i64 depth;
    jmp_buf stack[NNC_EXCEPTION_MAX_DEPTH];
    nnc_i64 exceptions[NNC_EXCEPTION_MAX_DEPTH];
} nnc_exception_stack;

extern nnc_exception_stack glob_exception_stack;

typedef enum _nnc_exception {
    NNC_CONTEXT_SAVED   = 0x00,
    NNC_UNHANDLED       = 0x01,
    NNC_UNINPLEMENTED   = 0x02
} nnc_exception;

nnc_bool nnc_try(nnc_i64 state);
void nnc_try_free();
void nnc_throw(nnc_exception exception);
nnc_bool nnc_catch(nnc_exception exception);
nnc_bool nnc_finally();

#endif