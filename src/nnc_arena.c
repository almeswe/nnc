#include "nnc_arena.h"

/**
 * @brief Allocates & initializes arena entry instance.
 * @param size Size of allocated memory block.
 * @param ptr Pointer to allocted block.
 * @return Pointer to allocated & initialized arena entry.
 */
static nnc_arena_entry* nnc_arena_entry_init(nnc_u64 size, nnc_heap_ptr ptr) {
    nnc_arena_entry* entry = (nnc_arena_entry*)
        calloc(1, sizeof(nnc_arena_entry));
    if (entry == NULL) {
        nnc_abort_no_ctx("nnc_arena_entry_init: calloc failed.\n");
    }
    entry->hptr = ptr;
    entry->bytes = size;
    return entry;
}

/**
 * @brief Finalizes arena entry instance.
 * @param entry Pointer to arena entry to be freed.
 */
static void nnc_arena_entry_fini(nnc_arena_entry* entry) {
    if (entry != NULL) {
        free(entry->hptr);
        entry->hptr = NULL;
        free(entry);
    }
}

/**
 * @brief Determines if arena needs compressing.
 * @param arena Pointer to arena instance.
 * @return `true` if arena needs compressing, otherwise `false`.
 */
static nnc_bool nnc_arena_need_zip(nnc_arena* arena) {
    assert(arena->metrics.disposed >= 0);
    if (arena->metrics.disposed == 0) {
        return false;
    }
    nnc_f64 rate = arena->metrics.disposed / 
        (nnc_f64)arena->metrics.cap;
    return rate >= NNC_ARENA_ZIP_RATE;
}

/**
 * @brief Performs memory fragmentation (zipping & compressing) 
 *  of existing entry pointer array.
 * @param arena Pointer to arena instance.
 */
static void nnc_arena_zip(nnc_arena* arena) {
    nnc_u64 initial = arena->metrics.len;
    // reset len metric, because it will be recalculated.
    arena->metrics.len = 0;
    // also reset disposed metric, because after fragmentation
    // there will be no logically disposed entries.
    arena->metrics.disposed = 0;
    for (nnc_u64 addr = 0; addr < initial; addr++) {
        if (arena->entries[addr] != NULL) {
            arena->entries[arena->metrics.len] = arena->entries[addr];
            if (arena->metrics.len != addr) {
                arena->entries[addr] = NULL;
            }
            arena->metrics.len++;
        }
    }
}

/**
 * @brief Extends arena's max capacity.
 * @param arena Pointer to arena instance.
 */
static void nnc_arena_grow(nnc_arena* arena) {
    arena->metrics.cap *= 2;
    nnc_u64 size = arena->metrics.cap 
        * sizeof(nnc_arena_entry*);
    arena->entries = (nnc_arena_entry**)
        realloc(arena->entries, size);
    if (arena->entries == NULL) {
        nnc_abort_no_ctx("nnc_arena_grow: realloc failed.\n");
    }
}

/**
 * @brief Pushes new entry to arena instance.
 *  Performs arena's growth and zip (compressing) if needed.
 * @param arena Pointer to arena instance.
 * @param entry Pointer to entry that must be pushed.
 */
static void nnc_arena_push(nnc_arena* arena, nnc_arena_entry* entry) {
    if (arena->metrics.cap == arena->metrics.len) {
        if (nnc_arena_need_zip(arena)) {
            nnc_arena_zip(arena);
        }
        else {
            nnc_arena_grow(arena);
        }
    }
    arena->alloc_bytes += entry->bytes;
    arena->entries[arena->metrics.len++] = entry;
}

/**
 * @brief Flushes entry from arena and updates arena's metrics.
 * @param arena Pointer to arena instance.
 * @param index Index of entry at `arena->entries` to be flushed.
 */
static void nnc_arena_flush(nnc_arena* arena, nnc_u64 index) {
    assert(arena->entries[index] != NULL);
    nnc_arena_entry* entry = arena->entries[index];
    // decrease number of allocated bytes
    arena->alloc_bytes -= entry->bytes;
    // and flush pointer to entry.
    arena->entries[index] = NULL;
}

/**
 * @brief Allocates memory inside arena.
 * @param size Size of memory block to be allocted.
 * @return Pointer to allocated memory block.
 */
nnc_heap_ptr nnc_alloc(nnc_u64 size) {
    nnc_heap_ptr mem = malloc(size);
    if (mem == NULL) {
        nnc_abort_no_ctx("nnc_alloc: malloc failed.\n");
    }
    // flush allocated memory.
    memset(mem, 0, size);
    // create new nnc_arena_entry, and push it the the arena.
    nnc_arena_push(&glob_arena, 
        nnc_arena_entry_init(size, mem));
    return mem;
}

/**
 * @brief Disposes and flushes memory located at address.
 *  Searches specified address in global arena, if this address
 *  is not listed there, nothing will happen.
 * @param ptr Pointer to memory which must be freed.
 */
void nnc_dispose(nnc_heap_ptr ptr) {
    for (nnc_u64 i = 0; i < glob_arena.metrics.len; i++) {
        nnc_arena_entry* entry = glob_arena.entries[i];
        // if address located at entry is the same as one specified.
        // it means that entry was found, and must be disposed.
        if (entry != NULL && entry->hptr == ptr) {
            // but before, flush memory itself
            memset(ptr, 0, entry->bytes);
            // decrease total allocated bytes in arena
            // and set this entry to NULL
            nnc_arena_flush(&glob_arena, i);
            // finalize entry itself
            nnc_arena_entry_fini(entry);
            // increase metric needed for 
            // memory fragmentation (see nnc_arena_zip function)            
            glob_arena.metrics.disposed++;
            break;
        } 
    }
}

/**
 * @brief Initializes stack-allocated memory arena instance.
 * @param out_arena Pointer to stack-allocated memory arena
 *  instance which will be initialized.
 */
void nnc_arena_init(nnc_arena* out_arena) {
    *out_arena = (nnc_arena){
        .alloc_bytes = 0,
        .entries = NULL,
        .metrics = {
            .len = 0,
            .cap = NNC_ARENA_CAP,
            .disposed = 0
        }
    };
    out_arena->entries = (nnc_arena_entry**)
        calloc(out_arena->metrics.cap, sizeof(nnc_arena_entry));
    if (out_arena->entries == NULL) {
        nnc_abort_no_ctx("nnc_arena_init: calloc failed.\n");
    }
}

/**
 * @brief Finalizes every allocated entry, and arena itself.
 * @param arena Pointer to stack-allocated memory arena instance.
 */
void nnc_arena_fini(nnc_arena* arena) {
    for (nnc_u64 i = 0; i < arena->metrics.len; i++) {
        nnc_arena_entry* entry = glob_arena.entries[i];
        if (entry != NULL) {
            nnc_arena_flush(arena, i);
            nnc_arena_entry_fini(entry);
        }
    }
    free(arena->entries);
    // reset values of arena instance
    arena->entries = NULL;
    arena->metrics.cap = 0;
    arena->metrics.len = 0;
    arena->metrics.disposed = 0;
    assert(arena->alloc_bytes == 0);
}