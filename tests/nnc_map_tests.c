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

int nncmap_test_dups() {
    nnc_arena_init(&glob_arena);
    nnc_map* str_map = map_init();

    map_put(str_map, "key1", "val1");
    map_put(str_map, "key2", "val2");
    NNC_ASSERT(map_has(str_map, "key1"));
    NNC_ASSERT(map_has(str_map, "key2"));
    NNC_ASSERT(str_map->len == 2);

    nnc_u64 allocd = glob_arena.alloc_bytes;
    map_put(str_map, "key2", "val2.2");
    NNC_ASSERT(map_has(str_map, "key2"));
    char* val = (char*)map_get(str_map, "key2");
    NNC_TEST_PRINTF("len: %lu\n", str_map->len);
    NNC_TEST_PRINTF("val: %s\n", val);
    NNC_ASSERT(str_map->len == 2);
    NNC_ASSERT(strcmp(val, "val2.2") == 0);
    NNC_ASSERT(glob_arena.alloc_bytes == allocd);

    map_fini(str_map);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int nncmap_test_pop() {
    nnc_arena_init(&glob_arena);
    nnc_map* str_map = map_init();

    map_put(str_map, "key1", "val1");
    map_pop(str_map, "key12");
    NNC_ASSERT(str_map->len == 1);
    map_pop(str_map, "key1");
    NNC_ASSERT(str_map->len == 0);
    map_put(str_map, "key1", "val1");
    NNC_ASSERT(str_map->len == 1);

    map_fini(str_map);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int nncmap_test_fini() {
    nnc_arena_init(&glob_arena);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_put(int_map, i, i*i);
    }
    NNC_ASSERT(int_map->len == 50000);
    NNC_TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    map_fini(int_map);
    NNC_TEST_PRINTF("after map_fini: %lu bytes\n", glob_arena.alloc_bytes);
    NNC_ASSERT(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int nncmap_test_put_n_get_n_pop_1k() {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_put(int_map, i, i*i);
    }
    NNC_ASSERT(int_map->len == 1024);
    for (nnc_u64 i = 0; i < 1024; i++) {
        NNC_ASSERT(i*i == (nnc_u64)map_get(int_map, i));
    }
    NNC_TEST_PRINTF("len: %lu\n", int_map->len);
    NNC_TEST_PRINTF("cap: %lu\n", int_map->cap);
    NNC_TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_pop(int_map, i);
    }
    NNC_TEST_PRINTF("after pop len: %lu\n", int_map->len);
    NNC_ASSERT(int_map->len == 0);
    map_fini(int_map);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int nncmap_test_put_n_get_n_pop_50k() {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_put(int_map, i, i*i);
    }
    NNC_ASSERT(int_map->len == 50000);
    for (nnc_u64 i = 0; i < 50000; i++) {
        NNC_ASSERT(i*i == (nnc_u64)map_get(int_map, i));
    }
    NNC_TEST_PRINTF("alloc'd: %lu bytes\n", glob_arena.alloc_bytes);
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_pop(int_map, i);
    }
    NNC_ASSERT(int_map->len == 0);
    map_fini(int_map);
    nnc_arena_fini(&glob_arena);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    NNC_TEST(nncmap_test_basic);
    NNC_TEST(nncmap_test_pop);
    NNC_TEST(nncmap_test_dups);
    NNC_TEST(nncmap_test_fini);
    NNC_TEST(nncmap_test_put_n_get_n_pop_1k);
    NNC_TEST(nncmap_test_put_n_get_n_pop_50k);
    nnc_test_stats();
    return EXIT_SUCCESS;
}