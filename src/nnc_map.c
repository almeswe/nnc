#include "nnc_map.h"

/**
 * @brief Hash function for key represented as 64bit number.
 * @param key 64bit number key to be hashed.
 * @return Hash of the key.
 */
nnc_map_hash nncmap_hash(nnc_map_key key) {
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
nnc_map_hash nncmap_hash_str(nnc_map_key key) {
    nnc_i32 c = 0;
    nnc_u64 hash = 5481;
    char* str_key = (char*)key;
    while ((c = *(str_key++))) {
        hash = ((hash << 5) + hash) + c;
    }
    return (nnc_map_hash)hash;
}

nnc_static nnc_bool nncmap_keys_cmp(nnc_map_key key1, nnc_map_key key2, map_hash_fn* hash) {
    if (hash == nncmap_hash_str) {
        return strcmp((const char*)key1, (const char*)key2) == 0;
    }
    return key1 == key2;
}

/**
 * @brief Initializes new instance of `nnc_map_bucket`
 * @return Allocated & initialized instance of `nnc_map_bucket`.
 */
nnc_static nnc_map_bucket* nncmap_bucket_init() {
    nnc_map_bucket* bucket = (nnc_map_bucket*)nnc_alloc(sizeof(nnc_map_bucket));
    bucket->key = 0;
    bucket->val = NULL;
    bucket->next = NULL;
    bucket->has_key = false;
    return bucket;
}

/**
 * @brief Finalizes instance of `nnc_map_bucket`
 * @param bucket Pointer to bucket instance to be deallocated.
 */
nnc_static void nncmap_bucket_fini(nnc_map_bucket* bucket) {
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

/**
 * @brief Finalizes array of buckets stored at `map->buckets`.
 * @param map Pointer to map instance which buckets will be deallocated.
 */
nnc_static void nncmap_buckets_fini(nnc_map* map) {
    for (nnc_u64 i = 0; i < map->cap; i++) {
        if (map->buckets[i].has_key) {
            nncmap_bucket_fini(&map->buckets[i]);
        }
    }
    nnc_dispose(map->buckets);
}

/**
 * @brief Determines if map needs rehashing.
 * @param map Pointer to map instance to check.
 * @return `true` if map needs rehashing, `false` otherwise.
 */
nnc_static nnc_bool nncmap_needs_rehash(nnc_map* map) {
    if (map->cap == 0) {
        return map->cap = 4, true;
    }
    nnc_f64 used = ((nnc_f64)map->len) / map->cap;
    return used >= NNC_MAP_MAX_LOAD;
}

/**
 * @brief Performs rehashing of the map.
 * @param map Pointer to map instance to rehash.
 * @param hash Hash function.
 */
nnc_static void nncmap_rehash(nnc_map* map, map_hash_fn* hash) {
    // copy initial map
    nnc_map copy = *map;
    // and then change the original map instance
    map->len = 0;
    map->cap = (nnc_u64)(map->cap * NNC_MAP_REHASH_SCALAR);
    // allocate new space which will be filled due rehashing
    map->buckets = (nnc_map_bucket*)nnc_alloc(sizeof(nnc_map_bucket) * map->cap);
    for (nnc_u64 i = 0; i < copy.cap; i++) {
        nnc_map_bucket* bucket = &copy.buckets[i]; 
        for (; bucket != NULL; bucket = bucket->next) {
            if (!bucket->has_key) {
                break;
            }
            // put bucket from initial buckets to newly allocated
            nncmap_put(map, bucket->key, bucket->val, hash);
        }
    }
    // dispose old buckets
    nncmap_buckets_fini(&copy);
}

/**
 * @brief Initializes new instance of `nnc_map`.
 * @param inicap Initial capacity.
 * @return Allocated & initialized instance of `nnc_map`. 
 */
nnc_map* nncmap_init(nnc_u64 inicap) {
    nnc_map* map = (nnc_map*)nnc_alloc(sizeof(nnc_map));
    map->len = 0;
    map->cap = inicap;
    // preallocate initial number of buckets.
    map->buckets = (nnc_map_bucket*)nnc_alloc(sizeof(nnc_map_bucket) * map->cap);
    return map;
}

/**
 * @brief Gets values from map by key.
 * @param map Pointer to map instance.
 * @param key Key by which search is performed.
 * @param hash Hash function.
 * @return Value mapped by specified key, 
 *  `Assertion failed` if there are no value by this key.
 */
nnc_map_val nncmap_get(nnc_map* map, nnc_map_key key, map_hash_fn* hash) {
    nnc_map_idx idx = hash(key) % map->cap;
    nnc_map_bucket* bucket = &map->buckets[idx];
    for (; bucket != NULL; bucket = bucket->next) {
        if (nncmap_keys_cmp(bucket->key, key, hash)) {
            break;
        }
    }
    assert(bucket->has_key);
    return bucket->val;
}

/**
 * @brief Pops out the value pointer
 * @param map Pointer to map instance.
 * @param key Key by which to search.
 * @param hash Hash function.
 */
void nncmap_pop(nnc_map* map, nnc_map_key key, map_hash_fn* hash) {
    nnc_map_idx idx = hash(key) % map->cap;
    if (!nncmap_has(map, key, hash)) {
        return;
    }
    nnc_map_bucket* prev = &map->buckets[idx];
    nnc_map_bucket* curr = prev->next;
    // check if bucket (head of linked list)
    // is bucket which must be deleted.
    if (nncmap_keys_cmp(prev->key, key, hash)) {
        // if so, make shallow copy from
        // next child, and dispose it.
        map->len--;
        if (prev->next != NULL) {
            *prev = *(curr);
            nnc_dispose(curr);
        }
        else {
            memset(prev, 0, sizeof(nnc_map_bucket));
        }
    }
    // otherwise search needed bucket in it's children.
    else {
        for (; prev != NULL; prev = curr, curr = curr->next) {
            if (nncmap_keys_cmp(curr->key, key, hash)) {
                map->len--;
                prev->next = curr->next;
                nnc_dispose(curr);
                break;
            }    
        }
    }
}

/**
 * @brief Puts value to map by with specified key.
 * @param map Pointer to map instance.
 * @param key Key with which value to be added.
 * @param val Value to be added by specified key.
 * @param hash Hash function.
 */
void nncmap_put(nnc_map* map, nnc_map_key key, nnc_map_val val, map_hash_fn* hash) {
    if (nncmap_needs_rehash(map)) {
        nncmap_rehash(map, hash);
    }
    nnc_map_idx idx = hash(key) % map->cap;
    nnc_map_bucket* bucket = &map->buckets[idx];
    for (;; bucket = bucket->next) {
        if (!bucket->has_key || nncmap_keys_cmp(bucket->key, key, hash)) {
            // increase length only in case
            // when bucket without any key is filled.
            // otherwise rewriting the same bucket
            // with new value will be threated as new bucket. 
            // (and lenth will be incremented)
            if (!bucket->has_key) {
                map->len++;
            }
            bucket->key = key;
            bucket->val = val;
            bucket->next = NULL;
            bucket->has_key = true;
            break;
        }
        if (bucket->next == NULL) {
            bucket->next = nncmap_bucket_init();
        }
    }
}

/**
 * @brief Iterates through map and calls callback function on each bucket.
 * @param map Pointer to map instance.
 * @param iter Callback function called each time when bucket met.
 */
void nnc_map_iter(nnc_map* map, map_iter_fn* iter) {
    nnc_map_bucket* bucks = map->buckets;
    for (nnc_u64 i = 0; i < map->cap; i++) {
        if (!bucks[i].has_key) {
            continue;
        }
        nnc_map_bucket* buck = &bucks[i];
        for (; buck != NULL; buck = buck->next) {
            assert(buck->val != NULL);
            iter(buck->key, buck->val);
        }
    }
}

/**
 * @brief Determines if value by key is stored in map.
 * @param map Pointer to map instance.
 * @param key Key with which to check.
 * @param hash Hash function.
 * @return `true` if map has value with this key, otherwise `false`.
 */
nnc_bool nncmap_has(nnc_map* map, nnc_map_key key, map_hash_fn* hash) {
    nnc_map_idx idx = hash(key) % map->cap;
    nnc_map_bucket* bucket = &map->buckets[idx];
    if (!bucket->has_key) {
        return false;
    }
    for (; bucket != NULL; bucket = bucket->next) {
        if (nncmap_keys_cmp(bucket->key, key, hash)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Finalizes instance of `nnc_map`.
 * @param map Pointer to map instance to be deallocated.
 */
void nncmap_fini(nnc_map* map) {
    if (map != NULL) {
        nncmap_buckets_fini(map);
        nnc_dispose(map);
    }
}