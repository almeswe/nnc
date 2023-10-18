#ifndef _NNC_TRY_CATCH_STACK_H
#define _NNC_TRY_CATCH_STACK_H

#include <setjmp.h>
#include "nnc_error.h"

#define NNC_EXCEPTION_MAX_DEPTH 16
#define NNC_ENABLE_TRY_BLOCK_LOGGING 0

#define NNC_EXC_DEPTH glob_exception_stack.depth
#define NNC_EXC_STACK glob_exception_stack.stack
#define NNC_EXC_ARRAY glob_exception_stack.exceptions

#if !NNC_ENABLE_TRY_BLOCK_LOGGING
    #define NNC_LOG_TRY_ENTER
    #define NNC_LOG_TRY_LEAVE
    #define NNC_LOG_TRY_FREED
#else
    #define NNC_LOG_TRY_ENTER                           \
        fprintf(stderr, "%s: try in(%ld)\n",            \
            __FUNCTION__, glob_exception_stack.depth);
    #define NNC_LOG_TRY_LEAVE                           \
        fprintf(stderr, "%s: try out(%ld)\n",           \
            __FUNCTION__, glob_exception_stack.depth);
    #define NNC_LOG_TRY_FREED                              \
        nnc_i64 __nnc_depth = glob_exception_stack.depth;  \
        fprintf(stderr, "nnc_try_free: depth %ld->%ld.\n", \
            __nnc_depth + 1, __nnc_depth);
#endif

#define TRY                     NNC_LOG_TRY_ENTER if (nnc_try(setjmp(NNC_EXC_STACK[NNC_EXC_DEPTH])))
#define ETRY                    NNC_LOG_TRY_LEAVE NNC_LOG_TRY_FREED nnc_try_free()
#define THROW(exception, ...)   nnc_throw((nnc_exception){ exception, #exception, __VA_ARGS__ })
#define CATCH(exception)        else if (nnc_catch(exception))
#define RETHROW                 nnc_throw(CATCHED)
#define CATCHALL                else
#define CATCHED                 glob_exception_stack.thrown
#define FINALLY

#define NNC_SHOW_CATCHED(ctx) nnc_error(sformat("%s: %s\n", CATCHED.repr, CATCHED.what), ctx);

typedef enum _nnc_exception_kind {
    NNC_CONTEXT_SAVED   = 0x00,
    NNC_UNHANDLED       = 0x01,
    NNC_UNIMPLEMENTED   = 0x02,

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
    NNC_NAME_ALREADY_DECLARED,
    NNC_TYPE_ALREADY_DECLARED,
    NNC_CANNOT_RESOLVE_TYPE,
    NNC_CANNOT_RESOLVE_REF_EXPR,
    NNC_CANNOT_RESOLVE_NOT_EXPR,
    NNC_CANNOT_RESOLVE_CAST_EXPR,
    NNC_CANNOT_RESOLVE_PLUS_EXPR,
    NNC_CANNOT_RESOLVE_MINUS_EXPR,
    NNC_CANNOT_RESOLVE_DEREF_EXPR,
    NNC_CANNOT_RESOLVE_SIZEOF_EXPR,
    NNC_CANNOT_RESOLVE_LENGTHOF_EXPR,
    NNC_CANNOT_RESOLVE_BW_NOT_EXPR,
    NNC_CANNOT_RESOLVE_DOT_EXPR,
    NNC_CANNOT_RESOLVE_CALL_EXPR,
    NNC_CANNOT_RESOLVE_SCOPE_EXPR,
    NNC_CANNOT_RESOLVE_INDEX_EXPR,
    NNC_CANNOT_RESOLVE_ADD_EXPR,
    NNC_CANNOT_RESOLVE_MUL_EXPR,
    NNC_CANNOT_RESOLVE_SHIFT_EXPR,
    NNC_CANNOT_RESOLVE_REL_EXPR,
    NNC_CANNOT_RESOLVE_BITWISE_EXPR,
    NNC_CANNOT_ASSIGN_EXPR,
    NNC_CANNOT_RESOLVE_TERNARY_EXPR,
    NNC_WRONG_ENUMERATOR_INITIALIZER
} nnc_exception_kind;

typedef struct _nnc_exception {
    nnc_exception_kind kind;    // represents type of exception
    const char* repr;           // represents string representation of exception type
    const char* what;           // represents optional error message attached to exception
    nnc_ctx where;              // represents context where exception occured
} nnc_exception;

typedef struct _nnc_exception_stack {
    volatile nnc_i64 depth;                         // represents current depth (length of `exceptions` array)
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