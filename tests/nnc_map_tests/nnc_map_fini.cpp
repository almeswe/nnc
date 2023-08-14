#include "../nnc_test.hpp"

TEST(nnc_map_tests, fini) {
    nnc_arena_init(&glob_arena);
    ASSERT_TRUE(glob_arena.alloc_bytes == 0);
    nnc_map* int_map = map_init();
    for (nnc_u64 i = 0; i < 50000; i++) {
        map_put(int_map, i, i*i);
    }
    ASSERT_TRUE(int_map->len == 50000);
    map_fini(int_map);
    ASSERT_TRUE(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}