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

static FILE* nnc_create_file(const char* path) {
    FILE* file = fopen(path, "w+");
    if (file == NULL) {
        nnc_abort_no_ctx(sformat("cannot create `%s`. "
            "[errno %d] %s.\n", path, errno, strerror(errno)));
    }
    return file;
}

static void nnc_write_blob(const char* path, FILE* fp, const nnc_blob_buf* blob) {
    size_t written = fwrite(blob->blob, 1, blob->len, fp);
    if (written != blob->len || errno != 0) {
        nnc_abort_no_ctx(sformat("cannot write data to `%s`. "
            "[errno %d] %s.\n", path, errno, strerror(errno)));
    }
}

static void nnc_compile(const nnc_assembly_file* asm_file) {
    char dot_s_file[MAX_PATH] = {0};
    char dot_o_file[MAX_PATH] = {0};
    sprintf(dot_s_file, "%s.s", glob_nnc_argv.output);
    sprintf(dot_o_file, "%s.o", glob_nnc_argv.output);
    // create output file and indicate if it is not possible
    FILE* out = nnc_create_file(glob_nnc_argv.output);
    fclose(out);
    FILE* dot_o = nnc_create_file(dot_o_file);
    fclose(dot_o);
    FILE* dot_s = nnc_create_file(dot_s_file);
    nnc_blob_buf asm_blob = nnc_build(*asm_file);
    nnc_write_blob(dot_s_file, dot_s, &asm_blob);
    fclose(dot_s);
    system(sformat("as --64 -o %s %s", dot_o_file, dot_s_file));
    system(sformat("ld -o %s %s", glob_nnc_argv.output, dot_o_file));
    remove(dot_s_file);
    remove(dot_o_file);
}

static nnc_i32 nnc_main(nnc_i32 argc, char* argv[]) {
    TRY {
        nnc_parse_argv(argc, argv);
        assert(buf_len(glob_nnc_argv.sources) == 1);
        assert(buf_len(glob_nnc_argv.objects) == 0);
        assert(buf_len(glob_nnc_argv.static_libs) == 0);
        assert(buf_len(glob_nnc_argv.shared_libs) == 0);
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
        nnc_assembly_file unbuilt = nnc_gen(code);
        nnc_compile(&unbuilt);
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