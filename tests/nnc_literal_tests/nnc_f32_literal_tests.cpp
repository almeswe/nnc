#include "../nnc_test.hpp"

TEST(nnc_literal_tests, f32_overflow) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_dbl_new("3.40282346638528859811704183484516925e+39F32");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
    }
    nnc_test_fini();
}

TEST(nnc_literal_tests, f32_max) {
    nnc_arena_init(&glob_arena);
    TRY {
        fliteral = nnc_dbl_new("3.40282346638528859811704183484516925e+38F32");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F32);
        ASSERT_FLOAT_EQ(fliteral->exact, 3.40282346638528859811704183484516925e+38);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_literal_tests, f32_min) {
    nnc_arena_init(&glob_arena);
    TRY {
        fliteral = nnc_dbl_new("1.17549435082228750796873653722224568e-38F32");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F32);
        ASSERT_FLOAT_EQ(fliteral->exact, 1.17549435082228750796873653722224568e-38);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_literal_tests, f32_zero) {
    nnc_arena_init(&glob_arena);
    TRY {
        fliteral = nnc_dbl_new("0.0f32");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F32);
        ASSERT_FLOAT_EQ(fliteral->exact, 0.0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_test_fini();
}