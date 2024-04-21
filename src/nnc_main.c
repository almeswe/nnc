#include "nnc_core.h"
#include <sys/time.h>

nnc_ast* glob_current_ast = NULL;
nnc_arena glob_arena = { 0 };
nnc_error_canarie glob_error_canarie = { 0 };
nnc_exception_stack glob_exception_stack = { 0 };

static struct timeval start, end;

static void nnc_help() {
    fprintf(stderr, 
        "nnc:   Not Named x86_64 Compiler (see more: https://github.com/almeswe/nnc)\n"
        "usage: ./nnc [options] [sources] [objects] [libs] \n\n"
        "sources:                  `.nnc` or `.source` files\n"
        "objects:                  `.o` compiled & assembled files\n"
        "libs:                     `.a` for static libs\n"
        "                          `.so` for shared libs \n"
        "options (short):\n"
        "  -o <file>               Set path for output file\n"
        "  -c                      Compile only; do not assemble or link \n"
        "  -g                      Generates debug symbols when assembling.\n"
        "options (long):\n"
        "  --help                  Display this information\n"
        "  --gen-debug             Generates debug symbols when assembling.\n"
        "  --compile               Compile only; do not assemble or link \n"
        "  --output <file>         Set path for output file\n"
        "  --dump-ast              Display abstract syntax tree of the program.\n"
        "  --dump-ir               Display intermediate representation of the program.\n"
        "  --dump-dest <file>      Set dump destination file; Use this option with `dump-ast` or `dump-ir`\n"
    );
    nnc_arena_fini(&glob_arena);
    exit(EXIT_SUCCESS);
}

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

static void nnc_start_count_time() {
    gettimeofday(&start, NULL); 
}

static void nnc_stop_count_time() {
    gettimeofday(&end, NULL);
    nnc_f64 elapsed = (end.tv_usec - start.tv_usec) / 1000.0;
    //fprintf(stderr, "[elapsed: %.3lfms]\n", elapsed);
}

static void nnc_create_file(const char* path) {
    FILE* file = fopen(path, "w+");
    if (file == NULL) {
        nnc_abort_no_ctx(sformat("cannot create `%s`. "
            "[errno %d] %s.\n", path, errno, strerror(errno)));
    }
    fclose(file);
}

static void nnc_write_blob(const char* path, const nnc_blob_buf* blob) {
    FILE* file = fopen(path, "w+");
    size_t written = fwrite(blob->blob, 1, blob->len, file);
    if (written != blob->len || errno != 0) {
        nnc_abort_no_ctx(sformat("cannot write data to `%s`. "
            "[errno %d] %s.\n", path, errno, strerror(errno)));
    }
    fclose(file);
}

static void nnc_remove_file(const char* path) {
    remove(path);
}

static nnc_blob_buf nnc_compile(const char* path) {
    nnc_check_for_errors();
    glob_current_ast = nnc_parse(path);
    nnc_check_for_errors();
    nnc_resolve(glob_current_ast);
    if (glob_nnc_argv.dump_ast) {
        nnc_dump_ast(glob_current_ast);
    }
    nnc_check_for_errors();
    if (glob_nnc_argv.dump_ir) {
        nnc_dump_3a_code(code);
    }
    // todo: finalization
    return nnc_build(nnc_gen(code));
}

static void nnc_assemble_and_link(const vector(nnc_blob_buf) compiled) {
    char* ld_params = " ";
    char dot_s[MAX_PATH] = {0};
    char dot_o[MAX_PATH] = {0};
    nnc_create_file(glob_nnc_argv.output);
    nnc_u64 size = buf_len(glob_nnc_argv.sources);
    for (nnc_u64 i = 0; i < size; i++) {
        memset(dot_s, 0, sizeof dot_s);
        memset(dot_o, 0, sizeof dot_o);
        sprintf(dot_s, "%s.s", glob_nnc_argv.sources[i]);
        sprintf(dot_o, "%s.o", glob_nnc_argv.sources[i]);
        nnc_create_file(dot_s);
        nnc_write_blob(dot_s, &compiled[i]);
        if (!glob_nnc_argv.compile) {
            nnc_create_file(dot_o);
            //todo: check retcode..
            system(sformat("as --64 %s -o %s %s", (glob_nnc_argv.gen_debug ? "-g" : ""), dot_o, dot_s));
        }
    }
    if (glob_nnc_argv.compile) {
        return;
    }
    for (nnc_u64 i = 0; i < size; i++) {
        memset(dot_o, 0, sizeof dot_o);
        sprintf(dot_o, "%s.o", glob_nnc_argv.sources[i]);
        ld_params = sformat("%s %s", ld_params, dot_o);
    }
    // link all object (.o) files
    for (nnc_u64 i = 0; i < buf_len(glob_nnc_argv.objects); i++) {
        ld_params = sformat("%s %s", ld_params, glob_nnc_argv.objects[i]);
    }
    // link all static library (.a) files
    for (nnc_u64 i = 0; i < buf_len(glob_nnc_argv.static_libs); i++) {
        ld_params = sformat("%s %s", ld_params, glob_nnc_argv.static_libs[i]);
    }
    // link all shared library (.so) files
    for (nnc_u64 i = 0; i < buf_len(glob_nnc_argv.shared_libs); i++) {
        ld_params = sformat("%s %s", ld_params, glob_nnc_argv.shared_libs[i]);
    }
    system(sformat("ld -o %s %s", glob_nnc_argv.output, ld_params));
    for (nnc_u64 i = 0; i < size; i++) {
        memset(dot_s, 0, sizeof dot_s);
        memset(dot_o, 0, sizeof dot_o);
        sprintf(dot_s, "%s.s", glob_nnc_argv.sources[i]);
        sprintf(dot_o, "%s.o", glob_nnc_argv.sources[i]);
        nnc_remove_file(dot_s);
        nnc_remove_file(dot_o);
    }
}

static nnc_i32 nnc_main(nnc_i32 argc, char* argv[]) {
    nnc_parse_argv(argc, argv);
    nnc_u64 size = buf_len(glob_nnc_argv.sources);
    if (glob_nnc_argv.help) {
        nnc_help();
    }
    TRY {
        vector(nnc_blob_buf) compiled = NULL;
        for (nnc_u64 i = 0; i < size; i++) {
            buf_add(compiled, nnc_compile(glob_nnc_argv.sources[i]));
        }
        nnc_assemble_and_link(compiled);
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