#ifndef _NNC_TRY_CATCH_STACK_H
#define _NNC_TRY_CATCH_STACK_H

#include <setjmp.h>
#include "nnc_error.h"

#define NNC_EXCEPTION_MAX_DEPTH 16

#define NNC_EXC_DEPTH glob_exception_stack.depth
#define NNC_EXC_STACK glob_exception_stack.stack
#define NNC_EXC_ARRAY glob_exception_stack.exceptions

#define TRY                     if (nnc_try(setjmp(NNC_EXC_STACK[NNC_EXC_DEPTH])))
#define ETRY                    nnc_try_free()
#define THROW(exception, ...)   nnc_throw((nnc_exception){ exception, #exception, __VA_ARGS__ })
#define CATCH(exception)        else if (nnc_catch(exception))
#define CATCHALL                else
#define CATCHED                 glob_exception_stack.thrown
#define FINALLY

#define nnc_show_catched(ctx) nnc_error(sformat("%s: %s", CATCHED.repr, CATCHED.what), ctx);

typedef enum _nnc_exception_kind {
    NNC_CONTEXT_SAVED   = 0x00,
    NNC_UNHANDLED       = 0x01,
    NNC_UNINPLEMENTED   = 0x02,

    NNC_OVERFLOW,

    NNC_LEX_BAD_FILE,
    NNC_LEX_BAD_ESC,
    NNC_LEX_BAD_CHR,
    NNC_LEX_BAD_STR,
    NNC_LEX_BAD_EXP,
    NNC_LEX_BAD_FLOAT,
    NNC_LEX_BAD_SUFFIX,
    NNC_SYNTAX,
    NNC_SEMANTIC,
    NNC_TABLE_MISS,
} nnc_exception_kind;

typedef struct _nnc_exception {
    nnc_exception_kind kind;    // represents type of exception
    const char* repr;           // represents string representation of exception type
    const char* what;           // represents optional error message attached to exception
    const void* data;           // represents optional data attached to exception
} nnc_exception;

typedef struct _nnc_exception_stack {
    nnc_i64 depth;                                  // represents current depth (length of `exceptions` array)
    jmp_buf stack[NNC_EXCEPTION_MAX_DEPTH];         // represents array of saved contexts (used by setjmp and longjmp), to allow nested try-catch statements
    nnc_i64 exceptions[NNC_EXCEPTION_MAX_DEPTH];    // represents array of exception types, reason is the same as for `stack` was.
    nnc_exception thrown;                           // represents last thrown exception, this field is used by `CATCHED` macro.
} nnc_exception_stack;

extern nnc_exception_stack glob_exception_stack;

nnc_bool nnc_try(nnc_i64 state);
void nnc_try_free();
void nnc_throw(nnc_exception exception);
nnc_bool nnc_catch(nnc_i64 exception);

#endif