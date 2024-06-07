#ifndef __NNC_HEAP_ARENA_H__
#define __NNC_HEAP_ARENA_H__

#include <assert.h>
#include "nnc_alloc.h"

#define NNC_ARENA_CAP       4
#define NNC_ARENA_ZIP_RATE  ((nnc_f64)0.15)

#define nnc_max(a, b) ((a) >= (b) ? (a) : (b))
#define nnc_min(a, b) ((a) <= (b) ? (a) : (b))
#define nnc_pow2(x) ((x != 0) && ((x & (x-1)) == 0))
#define nnc_arr_size(x) (sizeof(x) / sizeof(*(x)))

//todo: add section to each arena entry, to dispose certain data

typedef struct _nnc_arena_entry {
    nnc_u64 bytes;
    nnc_heap_ptr hptr;
} nnc_arena_entry;

typedef struct _nnc_arena {
    nnc_u64 alloc_bytes;
    nnc_u64 overall_alloc_bytes;
    struct _nnc_arena_metrics {
        nnc_u64 len;
        nnc_u64 cap;
        nnc_u64 disposed;
    } metrics;
    nnc_arena_entry** entries;
} nnc_arena;

extern void nnc_abort_no_ctx(const char* what);

void nnc_arena_init(nnc_arena* out_arena);
void nnc_arena_fini(nnc_arena* arena);

#endif 