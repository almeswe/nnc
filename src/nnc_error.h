#ifndef _NNC_ERROR_H
#define _NNC_ERROR_H

#include <errno.h> 
#include "nnc_ctx.h"

typedef nnc_bool nnc_error_canarie; 

typedef struct _nnc_file_cache {
    const char* path;
} nnc_file_cache;

typedef struct _nnc_diagnostics {
    char a;
} nnc_diagnostics;

typedef struct _nnc_warning {
    char placeholder;
} _nnc_warning;

typedef struct _nnc_error {
    char placeholder;
} _nnc_error;

extern nnc_error_canarie glob_error_canarie;

void nnc_warning(const char* what, const nnc_ctx* ctx);
void nnc_error(const char* what, const nnc_ctx* ctx);
void nnc_abort(const char* what, const nnc_ctx* ctx);

void nnc_error_no_ctx(const char* what);
void nnc_abort_no_ctx(const char* what);

void nnc_reset_canarie();
nnc_bool nnc_error_occured();

#endif