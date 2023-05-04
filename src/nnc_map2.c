#include "nnc_map2.h"

/**
 * @brief Hash function for key represented as 64bit number.
 * @param key 64bit number key to be hashed.
 * @return Hash of the key.
 */
nnc_map_hash nncmap_hash(nnc_u64 key) {
    key = (key ^ (key >> 31) ^ (key >> 62)) * 0x319642b2d24d8ec3UL;
    key = (key ^ (key >> 27) ^ (key >> 54)) * 0x96de1b173f119089UL;
    key = (key ^ (key >> 30) ^ (key >> 60));
    return (nnc_map_hash)key;
}

/**
 * @brief Hash function for key represented by string.
 *  Uses `djb2` algorithm for hashing.
 * @param key String key to be hashed.
 * @return Hash of the key.
 */
nnc_map_hash nncmap_hash_str(const char* key) {
    nnc_i32 c = 0;
    nnc_u64 hash = 5481;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return (nnc_map_hash)hash;
}

static nnc_bool nncmap_check(nnc_map* map, nnc_map_key key) {
    for (nnc_u64 i = 0; i < map->type; i++) {
        if (map->data[key+i] != 0) {
            return false;
        }
    }
    return true;
}

static nnc_bool nncmap_check_key(nnc_map* map, nnc_map_key key) {
    for (nnc_u64 i = 0; i < buf_len(map->keys); i++) {
        if (map->keys[i] == key) {
            return true;
        }
    }
    return false;
}

static nnc_bool nncmap_need_rehash(nnc_map* map) {
    double used = ((double)map->len) / map->cap;
    return used >= NNC_MAP_REHASH;
}

static void nncmap_rehash(nnc_map* map) {
    
}

nnc_map* nncmap_init(nnc_u64 type) {
    nnc_map* map = nnc_alloc(sizeof(nnc_map));
    map->len = 0;
    map->cap = NNC_MAP_INICAP;
    map->type = type;
    map->keys = NULL;
    map->data = nnc_alloc(map->cap * type);
    return map;
}

/*void nncmap_put(nnc_map* map, nnc_map_prim key, void* data) {
    nnc_map_key map_key = nncmap_hash(key) % map->cap;
    if (!nncmap_check(map, map_key)) {
        printf("collision occured at %lu", map_key);
        return;
    }
    if (nncmap_need_rehash(map)) {
        printf("map needs rehash, exit...");
        return;
        //nnc_map_rehash(map);
    }
    printf("put at: %ld\n", map_key);
    nnc_heap_ptr src = data; 
    nnc_heap_ptr dst = &map->data[map_key * map->type];
    memcpy(dst, src, map->type);
    buf_add(map->keys, key);
}

void* nncmap_get(nnc_map* map, nnc_map_prim key) {
    nnc_map_key map_key = nncmap_hash(key) % map->cap;
    if (!nncmap_check_key(map, key)) {
        printf("key was not registered, exit.");
        return NULL;
    }
    printf("get at: %ld\n", map_key);
    return &map->data[map_key * map->type];
}*/

void nncmap_fini(nnc_map* map) {
    if (map != NULL) {
        if (map->data) {
            nnc_dispose(map->data);
        }        
        buf_free(map->keys);
        nnc_dispose(map);
    }
}