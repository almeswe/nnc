#include "nnc_map.h"

/**
 * @brief Hash function for key represented as 64bit number.
 * @param key 64bit number key to be hashed.
 * @return Hash of the key.
 */
static nnc_map_hash nncmap_hash(nnc_map_key key) {
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
static nnc_map_hash nncmap_hash_str(const char* key) {
    nnc_i32 c = 0;
    nnc_u64 hash = 5481;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return (nnc_map_hash)hash;
}

static nnc_map_bucket* nncmap_bucket_init() {
    nnc_map_bucket* bucket = nnc_alloc(sizeof(nnc_map_bucket));
    bucket->key = 0;
    bucket->val = NULL;
    bucket->next = NULL;
    bucket->has_key = false;
    return bucket;
}

static void nncmap_bucket_fini(nnc_map_bucket* bucket) {
    if (bucket != NULL) {
        nnc_map_bucket* temp = NULL;
        nnc_map_bucket* curr = bucket->next;
        for (; curr != NULL;) {
            temp = curr->next;
            nnc_dispose(curr);
            curr = temp;
        }
    }
}

static void nncmap_buckets_fini(nnc_map* map) {
    for (nnc_u64 i = 0; i < map->cap; i++) {
        if (map->buckets[i].has_key) {
            nncmap_bucket_fini(&map->buckets[i]);
        }
    }
    nnc_dispose(map->buckets);
}

static nnc_bool nncmap_needs_rehash(nnc_map* map) {
    double used = ((double)map->len) / map->cap;
    return used >= NNC_MAP_MAX_LOAD;
}

static void nncmap_rehash(nnc_map* map) {
    nnc_map copy = *map;
    map->len = 0;
    map->cap *= NNC_MAP_REHASH_SCALAR;
    map->buckets = nnc_alloc(sizeof(nnc_map_bucket) * map->cap);
    for (nnc_u64 i = 0; i < copy.cap; i++) {
        nnc_map_bucket* bucket = &copy.buckets[i]; 
        for (; bucket != NULL; bucket = bucket->next) {
            if (!bucket->has_key) {
                break;
            }
            nncmap_put(map, bucket->key, bucket->val);
        }
    }
    nncmap_buckets_fini(&copy);
}

nnc_map* nncmap_init() {
    nnc_map* map = nnc_alloc(sizeof(nnc_map));
    map->len = 0;
    map->cap = NNC_MAP_INICAP;
    map->buckets = nnc_alloc(sizeof(nnc_map_bucket) * map->cap);
    return map;
}

void* nncmap_get(nnc_map* map, nnc_map_key key) {
    nnc_map_idx idx = nncmap_hash(key) % map->cap;
    // it ensures that nncmap_has is already called from outside
    // before nncmap_get call performed
    // this is needed to avoid returning any value from this function
    // in case when key is not listed, due to fact that we don't know
    // what default value is. (may be NULL is valid return value too?)
    assert(nncmap_has(map, key));
    nnc_map_bucket* bucket = &map->buckets[idx];
    for (; bucket != NULL; bucket = bucket->next) {
        if (bucket->key == key) {
            break;
        }
    }
    return bucket->val;
}

void nncmap_put(nnc_map* map, nnc_map_key key, nnc_map_val val) {
    if (nncmap_needs_rehash(map)) {
        nncmap_rehash(map);
    }
    nnc_map_idx idx = nncmap_hash(key) % map->cap;
    nnc_map_bucket* bucket = &map->buckets[idx];
    for (;; bucket = bucket->next) {
        if (!bucket->has_key) {
            bucket->key = key;
            bucket->val = val;
            bucket->next = NULL;
            bucket->has_key = true;
            map->len++;
            break;
        }
        if (bucket->next == NULL) {
            bucket->next = nncmap_bucket_init();
        }
    }
}

nnc_bool nncmap_has(nnc_map* map, nnc_map_key key) {
    nnc_map_idx idx = nncmap_hash(key) % map->cap;
    nnc_map_bucket* bucket = &map->buckets[idx];
    if (!bucket->has_key) {
        return false;
    }
    for (; bucket != NULL; bucket = bucket->next) {
        if (bucket->key == key) {
            return true;
        }
    }
    return false;
}

void nncmap_fini(nnc_map* map) {
    if (map != NULL) {
        nncmap_buckets_fini(map);
        nnc_dispose(map);
    }
}