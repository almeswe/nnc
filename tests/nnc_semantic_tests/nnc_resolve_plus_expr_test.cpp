#include "nnc_resolve_exported.hpp"

/*
*  Resolves plus expression in case of integral type. 
*/
TEST(nnc_semantic_tests, resolve_plus_expr_1) {
    contents = "+2i64";
    TRY {
        nnc_create_unary(contents);
        ETRY;
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

/*
*  Resolves plus expression in case of integral type. 
*/
TEST(nnc_semantic_tests, resolve_plus_expr_2) {
    contents = "+2u64";
    TRY {
        nnc_create_unary(contents);
        ETRY;
    }
    CATCHALL {
        FAIL();
    }    
    nnc_test_fini();
}

/*
*  Resolves plus expression in case of float type. 
*/
TEST(nnc_semantic_tests, resolve_plus_expr_3) {
    contents = "+2.0000f32";
    TRY {
        nnc_create_unary(contents);
        ETRY;
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

/*
*  Resolves plus expression in case of non numeric type. 
*/
TEST(nnc_semantic_tests, resolve_plus_expr_4) {
    contents = "+(1 as i32*)";
    TRY {
        nnc_create_unary(contents);
        FAIL();
        ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_PLUS_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}

/*
*  Resolves plus expression in case of non numeric type. 
*/
TEST(nnc_semantic_tests, resolve_plus_expr_5) {
    contents = "+(1 as fn():i32)";
    TRY {
        nnc_create_unary(contents);
        FAIL();
        ETRY;
    }
    CATCH (NNC_CANNOT_RESOLVE_PLUS_EXPR) {
    }
    CATCHALL {
        FAIL();
    }
    nnc_test_fini();
}