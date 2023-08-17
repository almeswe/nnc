#include "nnc_resolve_exported.hpp"

/*
*  Resolves deref expression in case of integral type. 
*/
TEST(nnc_semantic_tests, resolve_deref_expr_1) {
    contents = "*(0i32)";
    TRY {
        nnc_create_unary(contents);
        FAIL(); ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_DEREF_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_semantic_tests, resolve_deref_expr_2) {
    contents = "*(0u32)";
    TRY {
        nnc_create_unary(contents);
        FAIL(); ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_DEREF_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_semantic_tests, resolve_deref_expr_3) {
    contents = "*(0.0f32)";
    TRY {
        nnc_create_unary(contents);
        FAIL(); ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_DEREF_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_semantic_tests, resolve_deref_expr_4) {
    contents = "*(\'c\')";
    TRY {
        nnc_create_unary(contents);
        FAIL(); ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_DEREF_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}


TEST(nnc_semantic_tests, resolve_deref_expr_5) {
    contents = "*(\"123\")";
    TRY {
        nnc_create_unary(contents);
        ETRY;
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_semantic_tests, resolve_deref_expr_6) {
    contents = "*(0i32 as fn(): i32)";
    TRY {
        nnc_create_unary(contents);
        FAIL(); ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_DEREF_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_semantic_tests, resolve_deref_expr_7) {
    contents = "*(0i32 as i32*)";
    TRY {
        nnc_create_unary(contents);
        ETRY;
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

TEST(nnc_semantic_tests, resolve_deref_expr_8) {
    contents = "*(0i32 as i32[1])";
    TRY {
        nnc_create_unary(contents);
        ETRY;
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}