#ifndef __NNC_BLOB_H__
#define __NNC_BLOB_H__

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "nnc_arena.h"
#include "nnc_format.h"

#define FMEMOPEN_BUF_GROWTH 512

typedef struct _nnc_blob_buf {
    char* blob;
    nnc_u64 len;
    nnc_u64 cap;
} nnc_blob_buf;

void nnc_blob_buf_init(
    nnc_blob_buf* buf
);

void nnc_blob_buf_fini(
    nnc_blob_buf* buf
);

void nnc_blob_buf_grow(
    nnc_blob_buf* buf
);

void nnc_blob_buf_append(
    nnc_blob_buf* src,
    const nnc_blob_buf* buf
);

void nnc_blob_buf_putf(
    nnc_blob_buf* buf,
    const char* format,
    ...
);

void nnc_create_file(
    const char* path
);

FILE* nnc_create_file2(
    const char* path
);

void nnc_write_blob(
    const char* path,
    const nnc_blob_buf* blob
);

void nnc_remove_file(
    const char* path
);

#endif