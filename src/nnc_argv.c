#include "nnc_argv.h"
#include "nnc_state.h"

nnc_verbose glob_verbose = {0};

nnc_static nnc_option glob_nnc_opts[] = {
    { O_HELP,     "--help",      O_NO_ARG, &glob_argv.help,      true  },
    { O_VERBOSE,  "--verbose",   O_NO_ARG, &glob_argv.verbose,   true  },
    { O_OPTIMIZE, "--no-opt",    O_NO_ARG, &glob_argv.optimize,  false },
    { O_COMPILE,  "-c",          O_NO_ARG, &glob_argv.compile,   true  },
    { O_COMPILE,  "--compile",   O_NO_ARG, &glob_argv.compile,   true  },
    { O_DEBUG,    "-g",          O_NO_ARG, &glob_argv.gen_debug, true  },
    { O_DEBUG,    "--gen-debug", O_NO_ARG, &glob_argv.gen_debug, true  },
    { O_OUT_IR,   "--dump-ir",   O_NO_ARG, &glob_argv.dump_ir,   true  },
    { O_OUT_AST,  "--dump-ast",  O_NO_ARG, &glob_argv.dump_ast,  true  },
    { O_OUT,      "-o",          O_ONE_ARG },
    { O_OUT,      "--output",    O_ONE_ARG },
    { O_NONE }
};

nnc_static const char* nnc_glob_argv_get_arg(nnc_i32* index, nnc_i32 argc, char* const* argv) {
    if (*index + 1 < argc) {
        (*index)++;
        return argv[*index];
    }
    nnc_abort_no_ctx(sformat("option `%s` requires an argument.\n", argv[*index]));
    return NULL;
}

nnc_static const nnc_option* nnc_glob_argv_iter(nnc_i32 argc, char* const* argv) {
    static nnc_i32 index = 1;
    if (index >= argc) {
        return NULL;
    }
    nnc_i32 i = 0;
    nnc_option* opt = &glob_nnc_opts[i];
    while (opt->id != O_NONE) {
        opt = &glob_nnc_opts[i];
        if (nnc_strcmp(argv[index], opt->opt)) {
            if (opt->flag != NULL) {
                *opt->flag = opt->flag_value;
            }
            if (opt->opt_arg == O_ONE_ARG) {
                opt->arg = nnc_glob_argv_get_arg(&index, argc, argv);
            }
            break;
        }
        i++;
    }
    if (opt->id == O_NONE) {
        opt->arg = argv[index];
    }
    index++;
    return opt;
}

nnc_static nnc_bool nnc_is_source(const char* arg) {
    const char* ext = strrchr(arg, '.');
    if (nnc_strcmp(ext, EXT_NNC_SOURCE1) ||
        nnc_strcmp(ext, EXT_NNC_SOURCE2)) {
        buf_add(glob_argv.sources, arg);
        return true;
    }
    if (nnc_strcmp(ext, EXT_OBJ)) {
        buf_add(glob_argv.objects, arg);
        return true;
    }
    if (nnc_strcmp(ext, EXT_STATIC_LIB)) {
        buf_add(glob_argv.static_libs, arg);
        return true;
    }
    if (nnc_strcmp(ext, EXT_SHARED_LIB)) {
        buf_add(glob_argv.shared_libs, arg);
        return true;
    }
    return false;
}

nnc_static const nnc_argv* nnc_glob_argv_parse(nnc_i32 argc, char* const* argv) {
    const nnc_option* opt = NULL;
    while ((opt = nnc_glob_argv_iter(argc, argv)) != NULL) {
        if (opt == NULL) {
            break;
        }
        if (opt->id == O_OUT) {
            glob_argv.output = opt->arg;
        }
        // it means that current argument is not a listed
        // option and must be checked if it is source file
        if (opt->id == O_NONE && !nnc_is_source(opt->arg)) {
            nnc_abort_no_ctx(sformat("unknown option `%s`, see `--help`.\n", opt->arg));
        }
    }
    return &glob_argv;
}

void nnc_glob_argv_init(nnc_i32 argc, char* const* argv) {
    glob_argv = (nnc_argv) {
        .help      = false,
        .verbose   = false,
        .optimize  = true,
        .compile   = false,
        .gen_debug = false,
        .dump_ir   = false,
        .dump_ast  = false,
        .output    = O_OUT_DEF,
        .sources     = NULL,
        .objects     = NULL,
        .static_libs = NULL,
        .shared_libs = NULL,
    };
    nnc_glob_argv_parse(argc, argv);
}

void nnc_glob_argv_fini() {
    buf_free(glob_argv.sources);
    buf_free(glob_argv.objects);
    buf_free(glob_argv.static_libs);
    buf_free(glob_argv.shared_libs);
}