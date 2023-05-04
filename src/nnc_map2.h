#ifndef _NNC_MAP_2_H
#define _NNC_MAP_2_H

#include "nnc_buf.h"
#include <stdio.h>

#define NNC_MAP_INICAP 128
#define NNC_MAP_REHASH 0.7

typedef nnc_u64 nnc_map_key;
typedef nnc_u64 nnc_map_hash;
typedef nnc_u64 nnc_map_prim;
typedef nnc_heap_ptr nnc_map_val;
typedef nnc_byte* nnc_map_data;

typedef struct _nnc_map {
    nnc_u64 cap;
    nnc_u64 len;
    nnc_u64 type;
    nnc_map_key* keys;
    nnc_map_data data;
} nnc_map;

#define nncmap__hdr(map) ((nnc_map*)((char*)(map) - offsetof(nnc_map, data)))
#define nncmap__cap(map) nncmap__hdr(map)->cap
#define nncmap__len(map) nncmap__hdr(map)->len
#define nncmap__buf(map) nncmap__hdr(map)->data

#define nncmap__key(map, key) (nncmap_hash((nnc_u64)key) % nncmap__cap(map) * sizeof(*map))

#define nncmap__get(map, key)       map[nncmap__key(map, key)];
#define nncmap__put(map, key, val)  map[nncmap__key(map, key)] = val;

nnc_map_hash nncmap_hash(nnc_u64 key);
nnc_map_hash nncmap_hash_str(const char* key);

nnc_map* nncmap_init(nnc_u64 type);
void nncmap_fini(nnc_map* map);

#endif