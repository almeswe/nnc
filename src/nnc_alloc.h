#ifndef __NNC_ARENA_ALLOCATORS_H__
#define __NNC_ARENA_ALLOCATORS_H__

#include <string.h>
#include <stdlib.h>

#include "nnc_typedefs.h"

/**
 * @brief Macro which simplifies `nnc_alloc` call.
 * @param type Textual type representation.
 * @return Pointer to allocated memory block.
 */
#define nnc_new(type) (type*)nnc_alloc(sizeof(type))

/**
 * @brief Macro which simplifies `nnc_alloc` call.
 *  Allocates blocks of certain size.
 * @param type Textual type representation.
 * @param size Amount of blocks to allocate.
 * @return Pointer to allocated memory block.
 */
#define nnc_cnew(type, size) (type*)nnc_alloc(sizeof(type) * (size))

typedef void* nnc_heap_ptr;

/**
 * @brief Allocates memory inside arena.
 * @param size Size of memory block to be allocted.
 * @return Pointer to allocated memory block.
 */
nnc_heap_ptr nnc_alloc(
    nnc_u64 size
);

/**
 * @brief Disposes and flushes memory located at address.
 *  Searches specified address in global arena, if this address
 *  is not listed there, nothing will happen.
 * @param ptr Pointer to memory which must be freed.
 */
void nnc_dispose(
    nnc_heap_ptr ptr
);

#endif