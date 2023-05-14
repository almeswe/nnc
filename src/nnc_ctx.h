#ifndef _NNC_CONTEXT_H
#define _NNC_CONTEXT_H

#include <libgen.h>
#include "nnc_format.h"

typedef struct _nnc_ctx {
    nnc_u32 hint_ln;
    nnc_u32 hint_ch;
    nnc_u64 hint_sz;
    const char* fabs;
} nnc_ctx;

nnc_ctx* nnc_ctx_init(const char* fabs);
void nnc_ctx_fini(nnc_ctx* ctx);
char* nnc_ctx_tostr(const nnc_ctx* ctx);

#endif