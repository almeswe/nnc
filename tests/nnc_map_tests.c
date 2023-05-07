#include "nnc_test.h"

TEST(basic, nncmap) {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    assert(int_map->len == 0);
    assert(int_map->cap == NNC_MAP_INICAP);
    for (nnc_u64 i = 0; i < int_map->cap; i++) {
        assert(!int_map->buckets[i].next);
        assert(!int_map->buckets[i].has_key);
    }
    assert(!map_has(int_map, "key1"));
    map_put(int_map, "key1", 1);
    assert(int_map->len == 1);
    assert(map_has(int_map, "key1"));
    int value = (nnc_u64)map_get(int_map, "key1");    
    assert(value == 1);
    nnc_arena_fini(&glob_arena);
}

TEST(map_dups, nncmap) {
    nnc_arena_init(&glob_arena);
    nnc_map* str_map = map_init();

    map_put(str_map, "key1", "val1");
    map_put(str_map, "key2", "val2");
    assert(map_has(str_map, "key1"));
    assert(map_has(str_map, "key2"));
    assert(str_map->len == 2);

    nnc_u64 allocd = glob_arena.alloc_bytes;
    map_put(str_map, "key2", "val2.2");
    assert(map_has(str_map, "key2"));
    char* val = (char*)map_get(str_map, "key2");
    TEST_PRINTF("len: %lu\n", str_map->len);
    TEST_PRINTF("val: %s\n", val);
    assert(str_map->len == 2);
    assert(strcmp(val, "val2.2") == 0);
    assert(glob_arena.alloc_bytes == allocd);

    map_fini(str_map);
    assert(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}

TEST(map_pop, nncmap) {
    nnc_arena_init(&glob_arena);
    nnc_map* str_map = map_init();

    map_put(str_map, "key1", "val1");
    map_pop(str_map, "key12");
    assert(str_map->len == 1);
    map_pop(str_map, "key1");
    assert(str_map->len == 0);
    map_put(str_map, "key1", "val1");
    assert(str_map->len == 1);

    map_fini(str_map);
    assert(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}

TEST(map_fini, nncmap) {
    nnc_arena_init(&glob_arena);
    assert(glob_arena.alloc_bytes == 0);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_put(int_map, i, i*i);
    }
    assert(int_map->len == 50000);
    TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    map_fini(int_map);
    TEST_PRINTF("after map_fini: %lu bytes\n", glob_arena.alloc_bytes);
    assert(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}

TEST(map_put_n_get_n_pop_1k, nncmap) {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_put(int_map, i, i*i);
    }
    assert(int_map->len == 1024);
    for (nnc_u64 i = 0; i < 1024; i++) {
        assert(i*i == (nnc_u64)map_get(int_map, i));
    }
    TEST_PRINTF("len: %lu\n", int_map->len);
    TEST_PRINTF("cap: %lu\n", int_map->cap);
    TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_pop(int_map, i);
    }
    TEST_PRINTF("after pop len: %lu\n", int_map->len);
    assert(int_map->len == 0);
    map_fini(int_map);
    nnc_arena_fini(&glob_arena);
}

TEST(map_put_n_get_n_pop_50k, nncmap) {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_put(int_map, i, i*i);
    }
    assert(int_map->len == 50000);
    for (nnc_u64 i = 0; i < 50000; i++) {
        assert(i*i == (nnc_u64)map_get(int_map, i));
    }
    TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_pop(int_map, i);
    }
    assert(int_map->len == 0);
    map_fini(int_map);
    nnc_arena_fini(&glob_arena);
}