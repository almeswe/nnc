#include "../nnc_test.hpp"

TEST(nnc_map_tests, basic) {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    ASSERT_TRUE(int_map->len == 0);
    ASSERT_TRUE(int_map->cap == NNC_MAP_INICAP);
    for (nnc_u64 i = 0; i < int_map->cap; i++) {
        ASSERT_TRUE(!int_map->buckets[i].next);
        ASSERT_TRUE(!int_map->buckets[i].has_key);
    }
    ASSERT_TRUE(!map_has(int_map, "key1"));
    map_put(int_map, "key1", 1);
    ASSERT_TRUE(int_map->len == 1);
    ASSERT_TRUE(map_has(int_map, "key1"));
    int value = (nnc_u64)map_get(int_map, "key1");    
    ASSERT_TRUE(value == 1);
    nnc_arena_fini(&glob_arena);
}