#ifndef __NNC_MAP_H__
#define __NNC_MAP_H__

#include "nnc_arena.h"

#define NNC_MAP_INICAP          128
#define NNC_MAP_MAX_LOAD        0.7
#define NNC_MAP_REHASH_SCALAR   1.5

/*
    Macro just to make map type definition more readable
*/
#define _map_(keytype, valtype)      nnc_map*
#define dictionary(keytype, valtype) nnc_map*

#define map_init()              nncmap_init(NNC_MAP_INICAP)
#define map_init_with(exact)    nncmap_init(exact)
#define map_fini(map)           nncmap_fini(map)
#define map_has(map, key)       nncmap_has(map, (nnc_map_key)(key), nncmap_hash)
#define map_get(map, key)       nncmap_get(map, (nnc_map_key)(key), nncmap_hash)
#define map_pop(map, key)       nncmap_pop(map, (nnc_map_key)(key), nncmap_hash)
#define map_put(map, key, val)  nncmap_put(map, (nnc_map_key)(key), (nnc_map_val)(val), nncmap_hash)

#define map_has_s(map, key)       nncmap_has(map, (nnc_map_key)(key), nncmap_hash_str)
#define map_get_s(map, key)       nncmap_get(map, (nnc_map_key)(key), nncmap_hash_str)
#define map_pop_s(map, key)       nncmap_pop(map, (nnc_map_key)(key), nncmap_hash_str)
#define map_put_s(map, key, val)  nncmap_put(map, (nnc_map_key)(key), (nnc_map_val)(val), nncmap_hash_str)

typedef void* nnc_map_val;
typedef nnc_u64 nnc_map_idx;
typedef nnc_u64 nnc_map_key;
typedef nnc_u64 nnc_map_hash;

typedef void (map_iter_fn)(nnc_map_key, nnc_map_val);
typedef nnc_map_hash (map_hash_fn)(nnc_map_key);

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
nnc_map_hash nncmap_hash_str(nnc_map_key key);

nnc_map* nncmap_init(nnc_u64 inicap);
nnc_map_val nncmap_get(nnc_map* map, nnc_map_key key, map_hash_fn* hash);
void nncmap_pop(nnc_map* map, nnc_map_key key, map_hash_fn* hash);
void nncmap_put(nnc_map* map, nnc_map_key key, nnc_map_val val, map_hash_fn* hash);
void nnc_map_iter(nnc_map* map, map_iter_fn* iter);
nnc_bool nncmap_has(nnc_map* map, nnc_map_key key, map_hash_fn* hash);
void nncmap_fini(nnc_map* map);

#endif