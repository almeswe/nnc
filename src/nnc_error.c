#include "nnc_error.h"

nnc_static void nnc_show_ctx_fp(const nnc_ctx* ctx, FILE* fp) {
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

nnc_static void nnc_show_ctx(const nnc_ctx* ctx) {
    FILE* fp = fopen(ctx->fabs, "r");
    assert(fp != NULL);
    nnc_show_ctx_fp(ctx, fp);
}

void nnc_warning(const char* what, const nnc_ctx* ctx) {
    fprintf(stderr, "%s", "\033[1m\033[33m" "[warning] " "\033[0m");
    if (ctx != NULL) {
        fprintf(stderr, "\033[1m\033[36m" "%s " "\033[0m", nnc_ctx_tostr(ctx));
    }
    fprintf(stderr, "%s", what);
}

void nnc_error(const char* what, const nnc_ctx* ctx) {
    fprintf(stderr, "%s", "\033[1m\033[31m" "[error] " "\033[0m");
    if (ctx != NULL) {
        fprintf(stderr, "\033[1m\033[36m" "%s " "\033[0m", nnc_ctx_tostr(ctx));
    }
    fprintf(stderr, "%s", what);
    glob_error_canarie = true;
}

void nnc_abort(const char* what, const nnc_ctx* ctx) {
    fprintf(stderr, "%s", "\033[1m\033[31m" "[abort requested] " "\033[0m");
    nnc_error(what, ctx);
    nnc_arena_fini(&glob_arena);
    exit(EXIT_FAILURE);
}

void nnc_error_no_ctx(const char* what) {
    nnc_error(what, NULL);
}

void nnc_abort_no_ctx(const char* what) {
    nnc_abort(what, NULL);
}

void nnc_reset_canarie() {
    glob_error_canarie = false;
}

nnc_bool nnc_error_occured() {
    return glob_error_canarie;
}