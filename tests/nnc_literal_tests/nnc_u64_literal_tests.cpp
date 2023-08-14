#include "../nnc_test.hpp"

TEST(nnc_literal_tests, u64_b2_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("b10000000000000000000000000000000000000000000000000000000000000000u64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("b1111111111111111111111111111111111111111111111111111111111111111u64");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 18446744073709551615);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("b0u64");
        ASSERT_EQ(iliteral->base, 2);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u64_b8_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("o2000000000000000000000u64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("o1777777777777777777777u64");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 18446744073709551615);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("o0u64");
        ASSERT_EQ(iliteral->base, 8);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u64_b10_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("18446744073709551616u64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        iliteral = nnc_int_new("18446744073709551615u64");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 18446744073709551615);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("0u64");
        ASSERT_EQ(iliteral->base, 10);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}

TEST(nnc_literal_tests, u64_b16_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_int_new("x10000000000000000u64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();        
    }
    TRY {
        iliteral = nnc_int_new("xFFFFFFFFFFFFFFFFu64");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 18446744073709551615);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        iliteral = nnc_int_new("x0u64");
        ASSERT_EQ(iliteral->base, 16);
        ASSERT_FALSE(iliteral->is_signed);
        ASSERT_EQ(iliteral->suffix, SUFFIX_U64);
        ASSERT_EQ(iliteral->exact.u, 0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}