#include "../nnc_test.hpp"

TEST(nnc_map_tests, push_n_fetch_n_pop_1k) {
    nnc_arena_init(&glob_arena);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_put(int_map, i, i*i);
    }
    ASSERT_TRUE(int_map->len == 1024);
    for (nnc_u64 i = 0; i < 1024; i++) {
        ASSERT_TRUE(i*i == (nnc_u64)map_get(int_map, i));
    }
    for (nnc_u64 i = 0; i < 1024; i++) {
        map_pop(int_map, i);
    }
    ASSERT_TRUE(int_map->len == 0);
    map_fini(int_map);
    nnc_arena_fini(&glob_arena);
}