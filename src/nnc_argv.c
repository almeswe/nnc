#include "nnc_argv.h"

nnc_argv glob_argv = {0};

static nnc_i32 glob_nnc_opt_idx = 0;

static const char* glob_nnc_short_opt = 
    NNC_OPT_COMPILE
    NNC_OPT_GEN_DEBUG
    NNC_OPT_OUTPUT  ":"
;

static const struct option glob_nnc_long_opt[] = {
    { NNC_OPT_HELP_LONG,     no_argument, &glob_argv.help,     true },
    { NNC_OPT_VERBOSE_LONG,  no_argument, &glob_argv.verbose,  true },
    
    { NNC_OPT_DUMP_IR_LONG,   no_argument,       &glob_argv.dump_ir,   true },
    { NNC_OPT_DUMP_AST_LONG,  no_argument,       &glob_argv.dump_ast,  true },

    { NNC_OPT_COMPILE_LONG,   no_argument,       &glob_argv.compile,   NNC_OPT_COMPILE[0]   },
    { NNC_OPT_GEN_DEBUG_LONG, no_argument,       &glob_argv.gen_debug, NNC_OPT_GEN_DEBUG[0] },

    { NNC_OPT_OUTPUT_LONG,    required_argument, NULL, NNC_OPT_OUTPUT[0]         },
    { NNC_OPT_DUMP_DEST_LONG, required_argument, NULL, NNC_OPT_DUMP_DEST_LONG[0] },

    { 0, 0, 0, 0 }
};

nnc_static nnc_bool nnc_treat_unopt_arg(const char* arg) {
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

static void nnc_validate_argv() {
    nnc_bool dump_smth = false;
    nnc_bool just_compile = false; 
    if (glob_argv.help     || 
        glob_argv.dump_ast || 
        glob_argv.dump_ir) {
        dump_smth = true;
    }
    if (glob_argv.compile) {
        just_compile = true;
    }
    if (!just_compile && !dump_smth) {
        if (buf_len(glob_argv.sources) < 1) {
            nnc_abort_no_ctx("no source specified.\n");
        }
    }
}

const nnc_argv* nnc_parse_argv(nnc_i32 argc, char* const* argv) {
    glob_argv = (nnc_argv) {
        .output = NNC_ARGV_DEF_OUT,
        .sources = NULL,
        .objects = NULL,
        .static_libs = NULL,
        .shared_libs = NULL,
        .dump_ir = false,
        .dump_ast = false,
        .dump_dest = NNC_ARGV_DEF_FILE,
        .dump_ir_pat = NULL,
        .dump_ast_pat = NULL,
        .help = false,
        .verbose = false,
        .dump_with_color = false,
    };
    for (;;) {
        opterr = 0;
        nnc_i32 opt = getopt_long(argc, argv, glob_nnc_short_opt,
            glob_nnc_long_opt, &glob_nnc_opt_idx);
        if (opt == -1) {
            break;
        }
        if (opt == 0) {
            ;
        }
        else if (opt == *NNC_OPT_GEN_DEBUG) {
            glob_argv.gen_debug = true;
        }
        else if (opt == *NNC_OPT_COMPILE) {
            glob_argv.compile = true;
        }
        else if (opt == *NNC_OPT_OUTPUT) {
            glob_argv.output = optarg;
        }
        else if (opt == *NNC_OPT_DUMP_DEST_LONG) {
            glob_argv.dump_dest = fopen(optarg, "w+");
            if (glob_argv.dump_dest == NULL) {
                nnc_abort_no_ctx(sformat("cannot use `%s` as destination "
                    "for `" NNC_OPT_DUMP_DEST_LONG "`.\n", optarg));
            }
        }
        else if (opt == '?') {
            if (optopt == 0) {
                nnc_abort_no_ctx(sformat("unknown option `%s`, "
                    "see `--" NNC_OPT_HELP_LONG "`.\n", argv[optind - 1]));
            }
            //todo: handle case of unknown option more presise
            nnc_abort_no_ctx(sformat("bad use of option `%s`, "
                "see `--" NNC_OPT_HELP_LONG "`.\n", argv[optind - 1]));
            //for (nnc_u64 i = 0; i < strlen(glob_nnc_short_opt); i++) {
            //    if (isalpha(glob_nnc_short_opt[i]) && isalpha(optopt)) {
            //        if (glob_nnc_short_opt[i] == optopt) {
            //            nnc_abort_no_ctx(sformat("bad use of option `%c`, "
            //                "see `--" NNC_OPT_HELP_LONG "`.\n", optopt));
            //        }
            //    }
            //}
            //nnc_abort_no_ctx("failed to parse command line arguments.\n");
        }
    }
    for (; optind < argc; optind++) {
        if (!nnc_treat_unopt_arg(argv[optind])) {
            nnc_abort_no_ctx(sformat("unknown option `%s`, "
                "see `--" NNC_OPT_HELP_LONG "`.\n", argv[optind]));
        }
    }
    nnc_validate_argv(&glob_argv);
    return &glob_argv;
}