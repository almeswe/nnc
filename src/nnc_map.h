#ifndef _NNC_MAP_H
#define _NNC_MAP_H

#include "nnc_arena.h"

#define NNC_MAP_INICAP          128
#define NNC_MAP_MAX_LOAD        0.7
#define NNC_MAP_REHASH_SCALAR   1.5

/*
    Macro just to make map type definition more readable
*/
#define _map_(keytype, valtype)   nnc_map*

#define map_init()              nncmap_init(NNC_MAP_INICAP)
#define map_init_with(exact)    nncmap_init(exact)
#define map_fini(map)           nncmap_fini(map)
#define map_has(map, key)       nncmap_has(map, (nnc_map_key)(key))
#define map_get(map, key)       nncmap_get(map, (nnc_map_key)(key))
#define map_pop(map, key)       nncmap_pop(map, (nnc_map_key)(key))
#define map_put(map, key, val)  nncmap_put(map, (nnc_map_key)(key), (nnc_map_val)(val))

#define map_has_s(map, key)       map_has(map, nncmap_hash_str(key))
#define map_get_s(map, key)       map_get(map, nncmap_hash_str(key))
#define map_pop_s(map, key)       map_pop(map, nncmap_hash_str(key))
#define map_put_s(map, key, val)  map_put(map, nncmap_hash_str(key), (nnc_map_val)(val))

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

nnc_map_hash nncmap_hash(nnc_map_key key);
nnc_map_hash nncmap_hash_str(const char* key);

nnc_map* nncmap_init(nnc_u64 inicap);
nnc_map_val nncmap_get(nnc_map* map, nnc_map_key key);
void nncmap_pop(nnc_map* map, nnc_map_key key);
void nncmap_put(nnc_map* map, nnc_map_key key, nnc_map_val val);
nnc_bool nncmap_has(nnc_map* map, nnc_map_key key);
void nncmap_fini(nnc_map* map);

#endif