#include "nnc_error.h"

static void nnc_show_ctx_fp(const nnc_ctx* ctx, FILE* fp) {
    fseek(fp, 0, SEEK_SET);
    char linebuf[FORMAT_BUF_SIZE] = { 0 };
    for (nnc_i32 i = 1; i <= ctx->hint_ln; i++) {
        memset(linebuf, 0, sizeof linebuf);
        if (feof(fp)) {
            assert(false);
        }
        fgets(linebuf, sizeof linebuf - 1, fp);
    }
    fclose(fp);
}

static void nnc_show_ctx(const nnc_ctx* ctx) {
    FILE* fp = fopen(ctx->fabs, "r");
    assert(fp != NULL);
    nnc_show_ctx_fp(ctx, fp);
}

void nnc_report_warning(const char* what, const nnc_ctx* ctx) {

}

void nnc_report_error(const char* what, const nnc_ctx* ctx) {

}

void nnc_abort(const char* what, const nnc_ctx* ctx) {
    if (ctx != NULL) {
        fprintf(stderr, "%s ", nnc_ctx_tostr(ctx));
    }
    fprintf(stderr, "%s", what);
    exit(EXIT_FAILURE);
}

void nnc_abort_no_ctx(const char* what) {
    nnc_abort(what, NULL);
}