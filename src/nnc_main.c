#include "nnc_core.h"
#include <sys/time.h>

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

static struct timeval start, end;
static void nnc_start_count_time() {
    gettimeofday(&start, NULL); 
}

static void nnc_stop_count_time() {
    gettimeofday(&end, NULL);
    nnc_f64 elapsed = (end.tv_usec - start.tv_usec) / 1000.0;
    fprintf(stderr, "[elapsed: %.3lfms]\n", elapsed); 
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
        //nnc_ast_to_3a(ast, ast->st);
        #ifdef NNC_SHOW_MEMORY_INFO
            nnc_print_used_mem();
        #endif
        //data = nnc_3a_optimize_data(data);
        //nnc_dump_3a_data(stderr, data);
        //nnc_dump_3a_code(stderr, code);
        //nnc_3a_addr arg1 = nnc_3a_mkcgt();
        //nnc_3a_addr arg2 = nnc_3a_mki2(0xff, &i64_type);
        //nnc_3a_quad q = nnc_3a_mkquad1(
        //    OP_MUL, arg1, &u64_type, arg1, arg2
        //);
        //buf_add(code->quads, q);
        nnc_gen_code(code);
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
    nnc_start_count_time();
    nnc_i32 status = nnc_main(argc, argv);
    nnc_stop_count_time();
    nnc_arena_fini(&glob_arena);
    return status;
}