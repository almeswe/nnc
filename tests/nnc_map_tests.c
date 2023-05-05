#include "nnc_test.h"

nnc_arena glob_arena;

int nncmap_test_basic() {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    NNC_ASSERT(int_map->len == 0);
    NNC_ASSERT(int_map->cap == NNC_MAP_INICAP);
    for (nnc_u64 i = 0; i < int_map->cap; i++) {
        NNC_ASSERT(!int_map->buckets[i].next);
        NNC_ASSERT(!int_map->buckets[i].has_key);
    }
    NNC_ASSERT(!map_has(int_map, "key1"));
    map_put(int_map, "key1", 1);
    NNC_ASSERT(int_map->len == 1);
    NNC_ASSERT(map_has(int_map, "key1"));
    int value = (nnc_u64)map_get(int_map, "key1");    
    NNC_ASSERT(value == 1);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int nncmap_test_midsize() {
    nnc_arena_init(&glob_arena);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_put(int_map, i, i*i);
    }
    for (nnc_u64 i = 0; i < 1024; i++) {
        NNC_ASSERT(i*i == (nnc_u64)map_get(int_map, i));
    }
    NNC_ASSERT(int_map->len == 1024);
    NNC_TEST_PRINTF("len: %lu\n", int_map->len);
    NNC_TEST_PRINTF("cap: %lu\n", int_map->cap);
    NNC_TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    map_fini(int_map);
    NNC_TEST_PRINTF("after map_fini: %lu bytes\n", glob_arena.alloc_bytes);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int nncmap_test_bigbucks() {
    nnc_arena_init(&glob_arena);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 100000; i++) {
        map_put(int_map, i, i*i);
    }
    for (nnc_u64 i = 0; i < 100000; i++) {
        NNC_ASSERT(i*i == (nnc_u64)map_get(int_map, i));
    }
    NNC_ASSERT(int_map->len == 100000);
    NNC_TEST_PRINTF("len: %lu\n", int_map->len);
    NNC_TEST_PRINTF("cap: %lu\n", int_map->cap);
    NNC_TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    map_fini(int_map);
    NNC_TEST_PRINTF("after map_fini: %lu bytes\n", glob_arena.alloc_bytes);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    NNC_TEST(nncmap_test_basic);
    NNC_TEST(nncmap_test_midsize);
    NNC_TEST(nncmap_test_bigbucks);
    nnc_test_stats();
    return EXIT_SUCCESS;
}