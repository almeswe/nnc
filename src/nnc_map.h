#ifndef _NNC_MAP3_H
#define _NNC_MAP3_H

#include "nnc_arena.h"
#include <stdio.h>

#define NNC_MAP_INICAP          128
#define NNC_MAP_MAX_LOAD        0.7
#define NNC_MAP_REHASH_SCALAR   2

#define map_def(keytype, valtype)   nnc_map*
#define map_init()              nncmap_init()
#define map_fini(map)           nncmap_fini(map)
#define map_has(map, key)       nncmap_has(map, (nnc_map_key)(key))
#define map_get(map, key)       nncmap_get(map, (nnc_map_key)(key))
#define map_put(map, key, val)  nncmap_put(map, (nnc_map_key)(key), (nnc_map_val)(val))

typedef void* nnc_map_val;
typedef nnc_u64 nnc_map_idx;
typedef nnc_u64 nnc_map_key;
typedef nnc_u64 nnc_map_hash;

typedef struct _nnc_map_bucket {
    nnc_bool has_key;
    nnc_map_key key;
    nnc_map_val val;
    struct _nnc_map_bucket* next;
} nnc_map_bucket;

typedef struct _nnc_map {
    nnc_u64 cap;
    nnc_u64 len;
    nnc_map_bucket* buckets;
} nnc_map;

nnc_map* nncmap_init();
void* nncmap_get(nnc_map* map, nnc_map_key key);
void nncmap_put(nnc_map* map, nnc_map_key key, nnc_map_val val);
nnc_bool nncmap_has(nnc_map* map, nnc_map_key key);
void nncmap_fini(nnc_map* map);

#endif