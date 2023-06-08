#ifndef _NNC_ARENA_ALLOCATORS_H
#define _NNC_ARENA_ALLOCATORS_H

#include <string.h>
#include <stdlib.h>

#include "nnc_types.h"

#define new(type) (type*)nnc_alloc(sizeof(type))
#define rem(ptr)  nnc_dispose(ptr)

typedef void* nnc_heap_ptr;

nnc_heap_ptr nnc_alloc(nnc_u64 size);
void nnc_dispose(nnc_heap_ptr ptr);

#endif