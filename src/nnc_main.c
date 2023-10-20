#include "nnc_core.h"

nnc_arena glob_arena = { 0 };
nnc_error_canarie glob_error_canarie = { 0 };
nnc_exception_stack glob_exception_stack = { 0 };

static void nnc_check_for_errors() {
    if (nnc_error_occured()) {
        //fprintf(stderr, "used memory: %lu B.\n", 
        //    glob_arena.alloc_bytes);
        nnc_arena_fini(&glob_arena);
        nnc_abort_no_ctx(sformat("%ld error(s) detected.\n", glob_error_canarie));
    }
    nnc_reset_canarie();
}

static void nnc_print_used_mem() {
    printf("used mem: %lu\n", glob_arena.alloc_bytes);
}

/*
static nnc_i32 nnc_main(nnc_i32 argc, char** argv) {
    TRY {
        nnc_ast* ast = nnc_parse(argv[1]);
        nnc_check_for_errors();
        nnc_resolve(ast);
        nnc_check_for_errors();
        nnc_dump_ast(ast);
       ETRY;
    }
    CATCHALL {
        NNC_SHOW_CATCHED(&CATCHED.where);
    }
    nnc_check_for_errors();
    return EXIT_SUCCESS;
}
*/

static nnc_i32 nnc_main(nnc_i32 argc, char** argv) {
    TRY {
        nnc_ast* ast = nnc_parse(argv[1]);
        nnc_check_for_errors();
        nnc_resolve(ast);
        nnc_check_for_errors();
        #ifdef NNC_SHOW_MEMORY_INFO
            nnc_print_used_mem();
        #endif
        //nnc_dump_ast(ast);
        nnc_ast_to_3a(ast, ast->st);
        #ifdef NNC_SHOW_MEMORY_INFO
            nnc_print_used_mem();
        #endif
        nnc_dump_3a(stderr, sets);
        ETRY;
    }
    CATCHALL {
        NNC_SHOW_CATCHED(&CATCHED.where);
    }
    nnc_check_for_errors();
    return EXIT_SUCCESS;
}

nnc_i32 main(nnc_i32 argc, char** argv) {
    nnc_reset_canarie();
    nnc_arena_init(&glob_arena);
    nnc_i32 status = nnc_main(argc, argv);
    nnc_arena_fini(&glob_arena);
    return status;
}