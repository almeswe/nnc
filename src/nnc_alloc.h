#ifndef _NNC_ARENA_ALLOCATORS_H
#define _NNC_ARENA_ALLOCATORS_H

#include <stdlib.h>
#include <string.h>
#include "nnc_types.h"

typedef void* nnc_heap_ptr;

nnc_heap_ptr nnc_alloc(nnc_u64 size);
void nnc_dispose(nnc_heap_ptr ptr);

#endif