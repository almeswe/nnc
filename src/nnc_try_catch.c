#include "nnc_try_catch.h"

nnc_bool nnc_try(nnc_i64 state) {
    nnc_i64 depth = glob_exception_stack.depth;
    if (depth == NNC_EXCEPTION_MAX_DEPTH) {
        nnc_abort("nnc_try:depth exceeded.\n", NULL);
    }
    //nnc_i64 state = setjmp(glob_exception_stack.stack[depth]);
    glob_exception_stack.exceptions[depth] = state;
    if (state == NNC_CONTEXT_SAVED) {
        glob_exception_stack.depth++;
    }
    else {
        nnc_try_free();
    }
    return state == NNC_CONTEXT_SAVED;
}

void nnc_try_free() {
    glob_exception_stack.depth--;
}

void nnc_throw(nnc_exception exception) {
    nnc_i64 depth = glob_exception_stack.depth - 1;
    longjmp(glob_exception_stack.stack[depth], exception);
    nnc_abort("nnc_throw:longjmp was aborted", NULL);
}

nnc_bool nnc_catch(nnc_exception exception) {
    nnc_i64 depth = glob_exception_stack.depth + 1;
    return glob_exception_stack.exceptions[depth] == exception;
}

nnc_bool nnc_finally() {
    return true;
}