#include "../nnc_test.hpp"

TEST(nnc_literal_tests, f64_new) {
    nnc_arena_init(&glob_arena);
    TRY {
        nnc_dbl_new("1.79769313486231570814527423731704357e+309F64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        nnc_dbl_new("2.22507385850720138309023271733240406e-309F64");
        FAIL();
    }
    CATCH (NNC_OVERFLOW) {
        SUCCEED();
    }
    TRY {
        fliteral = nnc_dbl_new("1.79769313486231570814527423731704357e+308F64");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F64);
        ASSERT_EQ(fliteral->exact, 1.79769313486231570814527423731704357e+308L);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    TRY {
        fliteral = nnc_dbl_new("2.22507385850720138309023271733240406e-308LF64");
        ASSERT_EQ(fliteral->suffix, SUFFIX_F64);
        ASSERT_EQ(fliteral->exact, 2.22507385850720138309023271733240406e-308L);
        ETRY;
    }
    CATCH (NNC_OVERFLOW) {
        FAIL();
    }
    nnc_arena_fini(&glob_arena);
}