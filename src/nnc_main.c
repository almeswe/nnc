#include "nnc_core.h"
#include <sys/time.h>

nnc_ast* glob_current_ast = NULL;
nnc_arena glob_arena = { 0 };
nnc_error_canarie glob_error_canarie = { 0 };
nnc_exception_stack glob_exception_stack = { 0 };

#define nnc_ms(x) ((glob_v_data.x.tv_sec * 1000000 + glob_v_data.x.tv_usec) / 1000) 
#define nnc_verbose(...) fprintf(stderr, __VA_ARGS__)

typedef struct _nnc_unit {
    nnc_ast* ast;
    nnc_assembly_file impl;
    vector(nnc_ir_glob_sym) ir;
    vector(nnc_blob_buf) compiled
} nnc_unit;

nnc_static nnc_unit glob_current_unit = {0};

typedef struct _nnc_verbose_data {
    struct timeval tv_start;
    struct timeval tv_link;
    struct timeval tv_compile;
    struct timeval tv_assemble;
    struct timeval tv_overall;
    nnc_i32 as_code;
    nnc_i32 ld_code;
    char* ld_params;
    char* as_params;
} nnc_verbose_data;

nnc_static nnc_verbose_data glob_v_data = {0};

nnc_static void nnc_help() {
    fprintf(stderr, 
        "nnc:   Not Named x86_64 Compiler (see more: https://github.com/almeswe/nnc)\n"
        "usage: ./nnc [options] [sources] [objects] [libs] \n\n"
        "sources:                  `.nnc` or `.source` files\n"
        "objects:                  `.o` compiled & assembled files\n"
        "libs:                     `.a` for nnc_static libs\n"
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

nnc_static void nnc_check_for_errors() {
    if (nnc_error_occured()) {
        nnc_arena_fini(&glob_arena);
        nnc_abort_no_ctx(sformat("%ld error(s) detected.\n", glob_error_canarie));
    }
    nnc_reset_canarie();
}

nnc_static void nnc_create_file(const char* path) {
    FILE* file = fopen(path, "w+");
    if (file == NULL) {
        nnc_abort_no_ctx(sformat("cannot create `%s`. "
            "[errno %d] %s.\n", path, errno, strerror(errno)));
    }
    fclose(file);
}

nnc_static void nnc_write_blob(const char* path, const nnc_blob_buf* blob) {
    FILE* file = fopen(path, "w+");
    size_t written = fwrite(blob->blob, 1, blob->len, file);
    if (written != blob->len || errno != 0) {
        nnc_abort_no_ctx(sformat("cannot write data to `%s`. "
            "[errno %d] %s.\n", path, errno, strerror(errno)));
    }
    fclose(file);
}

nnc_static void nnc_remove_file(const char* path) {
    remove(path);
}

nnc_static nnc_i32 nnc_shell(const char* desc, const char* what) {
    char errbuf[256] = {0};
    if (system(NULL) == 0) {
        sprintf(errbuf, "%s: no shell available to proceed.\n", desc);
        nnc_abort_no_ctx(errbuf);
    }
    nnc_i32 ret = system(what);
    if (errno < 0) {
        sprintf(errbuf, "%s: cannot run shell. [errno: %d])\n", desc, errno);
        nnc_abort_no_ctx(errbuf);
    }
    if (ret != 0) {
        sprintf(errbuf, "%s: failed. [code: %d, errno: %d]\n", desc, ret, errno);
        nnc_abort_no_ctx(errbuf);
    }
    return ret;
}

nnc_static void nnc_compile_pass1(const char* source) {
    nnc_check_for_errors();
    glob_current_unit.ir = NULL;
    glob_current_unit.impl = (nnc_assembly_file){0};
    glob_current_unit.ast = nnc_parse(source);
    glob_current_asm_file = &glob_current_unit.impl;
    nnc_blob_buf_init(&glob_current_asm_file->data_segment_impl);
    nnc_check_for_errors();
}

nnc_static void nnc_compile_pass2() {
    nnc_ir_glob_sym sym;
    for (nnc_u64 i = 0; i < buf_len(glob_current_unit.ast->root); i++) {
        TRY {
            nnc_resolve_stmt(
                glob_current_unit.ast->root[i], 
                glob_current_unit.ast->st
            );
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(&CATCHED.where);
        }
        if (nnc_error_occured()) {
            continue;
        }
        sym = nnc_gen_ir(
            glob_current_unit.ast->root[i],
            glob_current_unit.ast->st
        );
        buf_add(glob_current_unit.ir, sym);
        nnc_gen(&sym);
    }
    nnc_check_for_errors();
}

nnc_static void nnc_compilation_fini() {
    //todo: free ast
    //todo: free ir syms
}

nnc_static void nnc_compile() {
    char dot_s[MAX_PATH] = {0};
    nnc_u64 size = buf_len(glob_argv.sources);
    glob_current_unit = (nnc_unit){0};
    for (nnc_u64 i = 0; i < size; i++) {
        nnc_compile_pass1(glob_argv.sources[i]);
        nnc_compile_pass2();
        buf_add(glob_current_unit.compiled, nnc_build(glob_current_unit.impl));
        sprintf(dot_s, "%s.s", glob_argv.sources[i]);
        nnc_create_file(dot_s);
        nnc_write_blob(dot_s, &glob_current_unit.compiled[i]);
        //todo: dump this to separate file
        if (glob_argv.dump_ast) {
            nnc_dump_ast(glob_current_unit.ast);
        }
        if (glob_argv.dump_ir) {
            nnc_dump_ir(glob_current_unit.ir);
        }
        nnc_compilation_fini();
    }
    gettimeofday(&glob_v_data.tv_compile, NULL);
}

nnc_static void nnc_assemble() {
    char dot_s[MAX_PATH] = {0};
    char dot_o[MAX_PATH] = {0};
    nnc_create_file(glob_argv.output);
    nnc_u64 size = buf_len(glob_argv.sources);
    for (nnc_u64 i = 0; i < size; i++) {
        memset(dot_s, 0, sizeof dot_s);
        memset(dot_o, 0, sizeof dot_o);
        sprintf(dot_s, "%s.s", glob_argv.sources[i]);
        sprintf(dot_o, "%s.o", glob_argv.sources[i]);
        nnc_create_file(dot_o);
        glob_v_data.as_params = sformat("as --64 %s -o %s %s",
            (glob_argv.gen_debug ? "-g" : ""), dot_o, dot_s);
        glob_v_data.as_code = nnc_shell("as", glob_v_data.as_params);
    }
    gettimeofday(&glob_v_data.tv_assemble, NULL);
}

nnc_static void nnc_link() {
    char* ld_params = " ";
    char dot_o[MAX_PATH] = {0};
    nnc_u64 size = buf_len(glob_argv.sources);
    for (nnc_u64 i = 0; i < size; i++) {
        memset(dot_o, 0, sizeof dot_o);
        sprintf(dot_o, "%s.o", glob_argv.sources[i]);
        ld_params = sformat("%s %s", ld_params, dot_o);
    }
    for (nnc_u64 i = 0; i < buf_len(glob_argv.objects); i++) {
        ld_params = sformat("%s %s", ld_params, glob_argv.objects[i]);
    }
    for (nnc_u64 i = 0; i < buf_len(glob_argv.static_libs); i++) {
        ld_params = sformat("%s %s", ld_params, glob_argv.static_libs[i]);
    }
    for (nnc_u64 i = 0; i < buf_len(glob_argv.shared_libs); i++) {
        ld_params = sformat("%s %s", ld_params, glob_argv.shared_libs[i]);
    }
    glob_v_data.ld_params = sformat("ld -o %s %s", glob_argv.output, ld_params);
    glob_v_data.ld_code = nnc_shell("ld", glob_v_data.ld_params);
    gettimeofday(&glob_v_data.tv_link, NULL);
}

nnc_static void nnc_cleanup() {
    char dot_s[MAX_PATH] = {0};
    char dot_o[MAX_PATH] = {0};
    nnc_u64 size = buf_len(glob_argv.sources);
    for (nnc_u64 i = 0; i < size; i++) {
        memset(dot_s, 0, sizeof dot_s);
        memset(dot_o, 0, sizeof dot_o);
        sprintf(dot_s, "%s.s", glob_argv.sources[i]);
        sprintf(dot_o, "%s.o", glob_argv.sources[i]);
        if (!glob_argv.compile && !glob_argv.gen_debug) {
            nnc_remove_file(dot_s);
        }
        nnc_remove_file(dot_o);
    }
}

nnc_static void nnc_verbose_show() {
    if (!glob_argv.verbose) {
        return;
    }
    nnc_verbose("You see this because you set --" NNC_OPT_VERBOSE_LONG " option.\n");
    nnc_verbose("- as (Assembler) shell string: %s\n", glob_v_data.as_params);
    nnc_verbose("- ld (Linker)    shell string: %s\n", glob_v_data.ld_params);
    nnc_verbose("---------------------\n");
    nnc_verbose("Source files (*.nnc | *.source):\n");
    for (nnc_u64 i = 0; i < buf_len(glob_argv.sources); i++) {
        nnc_verbose("  - %s\n", glob_argv.sources[i]);
    }
    if (buf_len(glob_argv.objects) > 0) {
        nnc_verbose("Object files (*.o):\n");
        for (nnc_u64 i = 0; i < buf_len(glob_argv.objects); i++) {
            nnc_verbose("  - %s\n", glob_argv.objects[i]);
        }
    }
    if (buf_len(glob_argv.shared_libs) > 0 ||
        buf_len(glob_argv.static_libs) > 0) {
        nnc_verbose("Libraries (*.so | *.a):\n");
        for (nnc_u64 i = 0; i < buf_len(glob_argv.shared_libs); i++) {
            nnc_verbose("  - %s\n", glob_argv.shared_libs[i]);
        }
        for (nnc_u64 i = 0; i < buf_len(glob_argv.static_libs); i++) {
            nnc_verbose("  - %s\n", glob_argv.static_libs[i]);
        }
    }
    nnc_verbose("---------------------\n");
    nnc_verbose("[+] Compiled:  %ld file(s) in %lums\n",
        buf_len(glob_argv.sources), 
        nnc_ms(tv_compile) - nnc_ms(tv_start)
    );
    if (!glob_argv.compile) {
        nnc_verbose("[+] Assembled: %ld file(s) (code: %d) in %lums\n", 
            buf_len(glob_argv.sources), 
            glob_v_data.as_code,
            nnc_ms(tv_assemble) - nnc_ms(tv_compile)
        );
        nnc_verbose("[+] Linked:    %ld file(s) (code: %d) in %lums\n", 
            buf_len(glob_argv.sources) +
            buf_len(glob_argv.objects) +
            buf_len(glob_argv.static_libs) +
            buf_len(glob_argv.shared_libs),
            glob_v_data.ld_code,
            nnc_ms(tv_link) - nnc_ms(tv_assemble)
        );
    }
    nnc_verbose("---------------------\n");
    if (!glob_argv.compile) {
        nnc_verbose("Executable located at: %s\n", glob_argv.output);
    }
    nnc_verbose("Total (without cleanup): %lums, Memory (overall): %luKiB\n",
        nnc_ms(tv_overall) - nnc_ms(tv_start),
        glob_arena.overall_alloc_bytes / 1024
    );
}

nnc_static nnc_i32 nnc_main(nnc_i32 argc, char* argv[]) {
    nnc_parse_argv(argc, argv);
    if (glob_argv.help) {
        nnc_help();
    }
    TRY {
        nnc_compile();
        if (!glob_argv.compile) {
            nnc_assemble(), nnc_link();
        }
        ETRY;
    }
    CATCHALL {
        NNC_SHOW_CATCHED(&CATCHED.where);
    }
    nnc_check_for_errors();
    nnc_cleanup();
    return EXIT_SUCCESS;
}

nnc_i32 main(nnc_i32 argc, char** argv) {
    nnc_reset_canarie();
    nnc_arena_init(&glob_arena);
    gettimeofday(&glob_v_data.tv_start, NULL);
    nnc_i32 status = nnc_main(argc, argv);
    gettimeofday(&glob_v_data.tv_overall, NULL);
    nnc_verbose_show();
    nnc_arena_fini(&glob_arena);
    return status;
}