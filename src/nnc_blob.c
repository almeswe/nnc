#include "nnc_blob.h"

void nnc_blob_buf_init(nnc_blob_buf* buf) {
    assert(buf->len == 0);
    assert(buf->cap == 0);
    assert(buf->blob == NULL);
    buf->len = 0;
    buf->cap = FMEMOPEN_BUF_GROWTH;
    buf->blob = (char*)nnc_alloc(buf->cap);
}

void nnc_blob_buf_fini(nnc_blob_buf* buf) {
    assert(buf->blob != NULL);
    nnc_dispose(buf->blob);
}

void nnc_blob_buf_grow(nnc_blob_buf* buf) {
    assert(buf->blob != NULL);
    nnc_blob_buf prev_buf = *buf;
    buf->cap += FMEMOPEN_BUF_GROWTH;
    buf->blob = (char*)nnc_alloc(buf->cap);
    memcpy(buf->blob, prev_buf.blob, prev_buf.cap);
    nnc_dispose(prev_buf.blob);
}

void nnc_blob_buf_append(nnc_blob_buf* src, const nnc_blob_buf* buf) {
    nnc_blob_buf prev_src = *src;
    src->cap += buf->cap;
    src->len += buf->len;
    src->blob = (char*)nnc_alloc(src->cap);
    memcpy(src->blob, prev_src.blob, prev_src.len);
    memcpy(src->blob + prev_src.len, buf->blob, buf->len);
    nnc_blob_buf_fini(&prev_src);
}

void nnc_blob_buf_putf(nnc_blob_buf* buf, const char* format, ...) {
    if (buf->cap == 0) {
        nnc_blob_buf_init(buf); 
    }
    assert(buf->blob != NULL);
    va_list args;
	va_start(args, format);
    char formatbuf[FMEMOPEN_BUF_GROWTH] = {0};
    nnc_u64 written = vsnprintf(formatbuf, sizeof formatbuf, format, args);
	va_end(args);
    if (written + buf->len >= buf->cap) {
        nnc_blob_buf_grow(buf);
    }
    memcpy(buf->blob + buf->len, formatbuf, written);
    buf->len += written;
}