#include "nnc_ctx.h"

nnc_ctx* nnc_ctx_init(const char* fabs) {
    nnc_ctx* ctx = nnc_alloc(sizeof(nnc_ctx));
    if (fabs != NULL) {
        ctx->fabs = fabs;
    }
    ctx->hint_char = 0;
    ctx->hint_line = 0;
    ctx->hint_size = 0;
    return ctx;
}

char* nnc_ctx_tostr(const nnc_ctx* ctx) {
    char ctxbuf[512] = { 0 };
    snprintf(ctxbuf, sizeof ctxbuf - 1, "[L: %d, P: %d]", 
        ctx->hint_line, ctx->hint_char);
    char* ctxstr = (char*)nnc_alloc(strlen(ctxbuf) + 1);
    strcpy(ctxstr, ctxbuf);
    return ctxstr; 
}