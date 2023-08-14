#include "../nnc_test.hpp"

TEST(nnc_map_tests, dups) {
    nnc_arena_init(&glob_arena);
    nnc_map* str_map = map_init();

    map_put(str_map, "key1", "val1");
    map_put(str_map, "key2", "val2");
    ASSERT_TRUE(map_has(str_map, "key1"));
    ASSERT_TRUE(map_has(str_map, "key2"));
    ASSERT_TRUE(str_map->len == 2);

    nnc_u64 allocd = glob_arena.alloc_bytes;
    map_put(str_map, "key2", "val2.2");
    ASSERT_TRUE(map_has(str_map, "key2"));
    char* val = (char*)map_get(str_map, "key2");
    ASSERT_TRUE(str_map->len == 2);
    ASSERT_TRUE(strcmp(val, "val2.2") == 0);
    ASSERT_TRUE(glob_arena.alloc_bytes == allocd);

    map_fini(str_map);
    ASSERT_TRUE(glob_arena.alloc_bytes == 0);
    nnc_arena_fini(&glob_arena);
}