#ifndef _NNC_CONTEXT_H
#define _NNC_CONTEXT_H

#include <stdio.h>
#include "nnc_arena.h"

typedef struct _nnc_ctx {
    nnc_u32 hint_line;
    nnc_u32 hint_char;
    nnc_u64 hint_size;
    const char* fabs;
} nnc_ctx;

nnc_ctx* nnc_ctx_init(const char* fabs);
char* nnc_ctx_tostr(const nnc_ctx* ctx);

#endif