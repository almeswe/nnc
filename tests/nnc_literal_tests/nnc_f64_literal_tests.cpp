#include "../nnc_test.hpp"

TEST(nnc_literal_tests, f64_overflow) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_dbl_new("1.79769313486231570814527423731704357e+309F64");
        ETRY; FAIL();
    }
    CATCH (NNC_OVERFLOW) {
    }
    nnc_test_fini();
}

TEST(nnc_literal_tests, f64_max) {
    nnc_arena_init(&glob_arena);
    TRY {
        fliteral = nnc_dbl_new("1.79769313486231570814527423731704357e+308F64");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F64);
        ASSERT_DOUBLE_EQ(fliteral->exact, 1.79769313486231570814527423731704357e+308L);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_test_fini();
}

/*TEST(nnc_literal_tests, f64_min) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_dbl_new("2.22507385850720138309023271733240406e-308F64");
        printf("new: %e\n exact: %e\n", DBL_MIN, fliteral->exact);
        ASSERT_EQ(fliteral->suffix, SUFFIX_F64);
        ASSERT_DOUBLE_EQ(fliteral->exact, 2.22507385850720138309023271733240406e-308F);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_test_fini();
}*/

TEST(nnc_literal_tests, f64_zero) {
    nnc_arena_init(&glob_arena);
    TRY {
        fliteral = nnc_dbl_new("0.0f64");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F64);
        ASSERT_DOUBLE_EQ(fliteral->exact, 0.0);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_test_fini();
}