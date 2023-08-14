#include "../nnc_test.hpp"

TEST(nnc_literal_tests, u32_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b100000000000000000000000000000000u32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b11111111111111111111111111111111u32");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 4294967295);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0u32");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u32_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o40000000000u32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o37777777777u32");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 4294967295);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0u32");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u32_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("4294967296u32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("4294967295u32");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 4294967295);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0u32");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u32_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x100000000u32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("xffffffffu32");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 4294967295);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0u32");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U32);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}