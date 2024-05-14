#include "nnc_core.h"

nnc_ast* glob_ast = NULL;
nnc_arena glob_arena = { 0 };
nnc_error_canarie glob_error_canarie = 0;
nnc_exception_stack glob_exception_stack = { 0 };

typedef struct _nnc_unit {
    nnc_ast* ast;
    nnc_assembly_file impl;
    vector(nnc_ir_glob_sym) ir;
    vector(nnc_blob_buf) compiled;
} nnc_unit;

nnc_static nnc_unit glob_current_unit = {0};

nnc_static void nnc_help() {
    fprintf(stderr, O_HELP_STR);
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
        if (!nnc_error_occured()) {
            nnc_ir_glob_sym sym = nnc_gen_ir(
                glob_current_unit.ast->root[i],
                glob_current_unit.ast->st
            );
            buf_add(glob_current_unit.ir, sym);
            nnc_gen(&sym);
        }
    }
    nnc_check_for_errors();
}

nnc_static void nnc_compilation_fini() {
    //todo: free ast
    //todo: free ir syms
}

nnc_static void nnc_compile() {
    char dot_star[MAX_PATH] = {0};
    glob_current_unit = (nnc_unit){0};
    for (nnc_u64 i = 0; i < buf_len(glob_argv.sources); i++) {
        nnc_compile_pass1(glob_argv.sources[i]);
        nnc_compile_pass2();
        buf_add(glob_current_unit.compiled, nnc_build(glob_current_unit.impl));
        sprintf(dot_star, "%s.s", glob_argv.sources[i]);
        nnc_create_file(dot_star);
        nnc_write_blob(dot_star, &glob_current_unit.compiled[i]);
        if (glob_argv.dump_ir) {
            sprintf(dot_star, "%s.ir", glob_argv.sources[i]);
            nnc_dump_ir(nnc_create_file2(dot_star), glob_current_unit.ir);
        }
        if (glob_argv.dump_ast) {
            sprintf(dot_star, "%s.ast", glob_argv.sources[i]);
            nnc_dump_ast(nnc_create_file2(dot_star), glob_current_unit.ast);
        }
        nnc_compilation_fini();
    }
    gettimeofday(&glob_verbose.tv_compile, NULL);
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
        glob_verbose.as_params = sformat("as --64 %s -o %s %s",
            (glob_argv.gen_debug ? "-g" : ""), dot_o, dot_s);
        glob_verbose.as_code = nnc_shell("as", glob_verbose.as_params);
    }
    gettimeofday(&glob_verbose.tv_assemble, NULL);
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
    glob_verbose.ld_params = sformat("ld -o %s %s", glob_argv.output, ld_params);
    glob_verbose.ld_code = nnc_shell("ld", glob_verbose.ld_params);
    gettimeofday(&glob_verbose.tv_link, NULL);
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
    fprintf(stderr, "You see this because you set --verbose option.\n");
    fprintf(stderr, "- as (Assembler) shell string: %s\n", glob_verbose.as_params);
    fprintf(stderr, "- ld (Linker)    shell string: %s\n", glob_verbose.ld_params);
    fprintf(stderr, "---------------------\n");
    fprintf(stderr, "Source files (*.nnc | *.source):\n");
    for (nnc_u64 i = 0; i < buf_len(glob_argv.sources); i++) {
        fprintf(stderr, "  - %s\n", glob_argv.sources[i]);
    }
    if (buf_len(glob_argv.objects) > 0) {
        fprintf(stderr, "Object files (*.o):\n");
        for (nnc_u64 i = 0; i < buf_len(glob_argv.objects); i++) {
            fprintf(stderr, "  - %s\n", glob_argv.objects[i]);
        }
    }
    if (buf_len(glob_argv.shared_libs) > 0 ||
        buf_len(glob_argv.static_libs) > 0) {
        fprintf(stderr, "Libraries (*.so | *.a):\n");
        for (nnc_u64 i = 0; i < buf_len(glob_argv.shared_libs); i++) {
            fprintf(stderr, "  - %s\n", glob_argv.shared_libs[i]);
        }
        for (nnc_u64 i = 0; i < buf_len(glob_argv.static_libs); i++) {
            fprintf(stderr, "  - %s\n", glob_argv.static_libs[i]);
        }
    }
    fprintf(stderr, "---------------------\n");
    fprintf(stderr, "[+] Compiled:  %ld file(s) in %lums\n",
        buf_len(glob_argv.sources), 
        tv_ms(tv_compile) - tv_ms(tv_start)
    );
    if (!glob_argv.compile) {
        fprintf(stderr, "[+] Assembled: %ld file(s) (code: %d) in %lums\n", 
            buf_len(glob_argv.sources), 
            glob_verbose.as_code,
            tv_ms(tv_assemble) - tv_ms(tv_compile)
        );
        fprintf(stderr, "[+] Linked:    %ld file(s) (code: %d) in %lums\n", 
            buf_len(glob_argv.sources) +
            buf_len(glob_argv.objects) +
            buf_len(glob_argv.static_libs) +
            buf_len(glob_argv.shared_libs),
            glob_verbose.ld_code,
            tv_ms(tv_link) - tv_ms(tv_assemble)
        );
    }
    fprintf(stderr, "---------------------\n");
    if (!glob_argv.compile) {
        fprintf(stderr, "Executable located at: %s\n", glob_argv.output);
    }
    fprintf(stderr, "Total (without cleanup): %lums, Memory (overall): %luKiB\n",
        tv_ms(tv_overall) - tv_ms(tv_start),
        glob_arena.overall_alloc_bytes / 1024
    );
}

nnc_static nnc_i32 nnc_main() {
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
    nnc_glob_argv_init(argc, argv);
    gettimeofday(&glob_verbose.tv_start, NULL);
    nnc_i32 status = nnc_main();
    gettimeofday(&glob_verbose.tv_overall, NULL);
    nnc_verbose_show();
    nnc_glob_argv_fini(argc, argv);
    nnc_arena_fini(&glob_arena);
    return status;
}