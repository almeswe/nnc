#include "nnc_core.h"
#include <sys/time.h>

nnc_ast* glob_current_ast = NULL;
nnc_arena glob_arena = { 0 };
nnc_error_canarie glob_error_canarie = { 0 };
nnc_exception_stack glob_exception_stack = { 0 };

static void nnc_check_for_errors() {
    if (nnc_error_occured()) {
        nnc_arena_fini(&glob_arena);
        nnc_abort_no_ctx(sformat("%ld error(s) detected.\n", glob_error_canarie));
    }
    nnc_reset_canarie();
}

static void nnc_print_used_mem() {
    fprintf(stderr, "used mem: %lu\n", glob_arena.alloc_bytes);
}

static struct timeval start, end;
static void nnc_start_count_time() {
    gettimeofday(&start, NULL); 
}

static void nnc_stop_count_time() {
    gettimeofday(&end, NULL);
    nnc_f64 elapsed = (end.tv_usec - start.tv_usec) / 1000.0;
    //fprintf(stderr, "[elapsed: %.3lfms]\n", elapsed); 
}

static nnc_i32 nnc_main(nnc_i32 argc, char* argv[]) {
    TRY {
        nnc_parse_argv(argc, argv);
        glob_current_ast = nnc_parse(glob_nnc_argv.sources[0]);
        nnc_check_for_errors();
        nnc_resolve(glob_current_ast);
        if (glob_nnc_argv.dump_ast) {
            nnc_dump_ast(glob_current_ast);
        }
        nnc_check_for_errors();
        if (glob_nnc_argv.dump_ir) {
            nnc_dump_3a_code(code);
        }
        nnc_assembly_file out_asm = nnc_gen(code);
        fprintf(stderr, "%s", nnc_build(out_asm).blob);
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