#ifndef __NNC_CONTEXT_H__
#define __NNC_CONTEXT_H__

#include <libgen.h>
#include "nnc_format.h"

#ifndef MAX_PATH
    #define MAX_PATH 260
#endif

typedef struct _nnc_ctx {
    nnc_u16 hint_ln;
    nnc_u32 hint_ch;
    nnc_u16 hint_sz;
    const char* fabs;
} nnc_ctx;

nnc_ctx* nnc_ctx_init(const char* fabs);
void nnc_ctx_fini(nnc_ctx* ctx);
char* nnc_ctx_tostr(const nnc_ctx* ctx);

#endif