#ifndef _NNC_HEAP_ARENA_H
#define _NNC_HEAP_ARENA_H

#include <assert.h>
#include "nnc_alloc.h"

#define NNC_ARENA_CAP       4
#define NNC_ARENA_ZIP_RATE  ((nnc_f64)0.15)

typedef struct _nnc_arena_entry {
    nnc_u64 bytes;
    nnc_heap_ptr hptr;
} nnc_arena_entry;

typedef struct _nnc_arena {
    nnc_u64 alloc_bytes;
    struct _nnc_arena_metrics {
        nnc_u64 len;
        nnc_u64 cap;
        nnc_u64 disposed;
    } metrics;
    nnc_arena_entry** entries;
} nnc_arena;

extern nnc_arena glob_arena;
extern void nnc_abort_no_ctx(const char* what);

void nnc_arena_init(nnc_arena* out_arena);
void nnc_arena_fini(nnc_arena* arena);

#endif 