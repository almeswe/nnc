#include "nnc_ctx.h"

nnc_ctx* nnc_ctx_init(const char* fabs) {
    nnc_ctx* ctx = (nnc_ctx*)nnc_alloc(sizeof(nnc_ctx));
    ctx->fabs = NULL;
    if (fabs != NULL) {
        ctx->fabs = fabs;
    }
    ctx->hint_ch = 0;
    ctx->hint_ln = 0;
    ctx->hint_sz = 0;
    return ctx;
}

void nnc_ctx_fini(nnc_ctx* ctx) {
    if (ctx != NULL) {
        nnc_dispose(ctx);
    }
}

char* nnc_ctx_tostr(const nnc_ctx* ctx) {
    if (ctx->fabs && strlen(ctx->fabs) < MAX_PATH) {
        char path[MAX_PATH] = { 0 };
        strcpy(path, ctx->fabs);
        const char* fname = basename(path);
        return sformat("%s:%lu:%lu", ctx->fabs, 
            ctx->hint_ln, ctx->hint_ch);
    }
    return sformat("%lu:%lu", 
        ctx->hint_ln, ctx->hint_ch); 
}