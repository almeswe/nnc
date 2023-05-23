#include "nnc_try_catch.h"

/**
 * @brief Performs behaviour as `try` statement has. 
 * @param state Returned value from `setjmp` call. If 0, try statement was executed first time,
 *  otherwise this state represents type of exception which was thrown
 * @return `true` if try statement was executed first time, otherwise `false` 
 *  (means when it called after `longjmp` call) 
 */
nnc_bool nnc_try(nnc_i64 state) {
    nnc_i64 depth = glob_exception_stack.depth;
    if (depth == NNC_EXCEPTION_MAX_DEPTH) {
        nnc_abort("nnc_try:depth exceeded.\n", NULL);
    }
    // set state to last object inside stack
    glob_exception_stack.exceptions[depth] = state;
    // if it was called first time, it means
    // that we increase depth of try-catch statements
    if (state == NNC_CONTEXT_SAVED) {
        glob_exception_stack.depth++;
    }
    // otherwise, it means that this function was called after 
    // `nnc_throw` was called, so decrease depth, because we leaved try-catch closure.
    else {
        nnc_try_free();
    }
    return state == NNC_CONTEXT_SAVED;
}

/**
 * @brief Performs releasing data after try-catch closure.
 */
void nnc_try_free() {
    glob_exception_stack.depth--;
}

/**
 * @brief Performs behaviour as `throw` statement has.
 * @param exception Exception object to be thrown.
 */
void nnc_throw(nnc_exception exception) {
    nnc_i64 depth = glob_exception_stack.depth - 1;
    glob_exception_stack.thrown = exception;
    // performs long jump to last `nnc_try` call in stack.
    longjmp(glob_exception_stack.stack[depth], exception.kind);
    // if for some reason it did not jumped, abort, something weird happened.
    nnc_abort("nnc_throw:longjmp was aborted", NULL);
}

/**
 * @brief Performs behaviour as `catch` statement has.
 * @param exception Type of exception to be checked for.
 * @return `true` if thrown exception has specified type, otherwise `false` is returned.
 */
nnc_bool nnc_catch(nnc_i64 exception) {
    nnc_i64 depth = glob_exception_stack.depth + 1;
    return glob_exception_stack.exceptions[depth] == exception;
}