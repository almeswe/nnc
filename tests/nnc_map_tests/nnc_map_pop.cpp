#include "../nnc_test.hpp"

TEST(nnc_map_tests, pop) {
    nnc_arena_init(&glob_arena);
    nnc_map* str_map = map_init();

    map_put(str_map, "key1", "val1");
    map_pop(str_map, "key12");
    ASSERT_TRUE(str_map->len == 1);
    map_pop(str_map, "key1");
    ASSERT_TRUE(str_map->len == 0);
    map_put(str_map, "key1", "val1");
    ASSERT_TRUE(str_map->len == 1);

    map_fini(str_map);
    ASSERT_TRUE(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}
