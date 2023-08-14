#include "nnc_core.h"

nnc_arena glob_arena;
nnc_error_canarie glob_error_canarie;
nnc_exception_stack glob_exception_stack;
nnc_deferred_stack glob_deferred_stack;

#include <fcntl.h>

static void test() {
    char buf[64] = {0};
    sprintf(buf, "__nnctemp%d", rand());
    int fp = open(buf, O_CREAT | O_RDWR);
    if (fp == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    close(fp);
    FILE* file = fopen(buf, "w+");
    const char* a = 
        "fn main(): i32 {\n"
        "   return 0 as i32;\n"
        "}\n"
    ;
    fputs(a, file);
    fclose(file);
    remove(buf);
}

static nnc_i32 nnc_main(nnc_i32 argc, char** argv) {
    /**/
    TRY {
        /*nnc_parser parser = { 0 };
        nnc_parser_init(&parser, argv[1]);
        nnc_expression* expr = nnc_parse_expr(&parser);
        if (nnc_error_occured()) {
            return EXIT_FAILURE;
        }
        printf("%s\n", nnc_type_tostr(nnc_expr_infer_type(expr)));
        */
        nnc_ast* ast = nnc_parse(argv[1]);
        if (argc == 2) {
            while (glob_deferred_stack.entities->len > 0) {
                nnc_deferred_stack_resolve(&glob_deferred_stack);
            }
        }
        nnc_dump_ast(ast);
        /*
        if (ast->expr->kind == EXPR_INT_LITERAL) {
            const nnc_int_literal* literal = int_literal(ast->expr);
            printf("base: %d, ", literal->base);
            printf("suffix: %d, ", literal->suffix);
            if (literal->is_signed) {
                printf("val: %ld\n", literal->exact.d);
            }
            else {
                printf("val: %lu\n", literal->exact.u);
            }
        }
        if (ast->expr->kind == EXPR_DBL_LITERAL) {
            const nnc_dbl_literal* literal = dbl_literal(ast->expr);
            printf("suffix: %d, ", literal->suffix);
            printf("val: %f\n", literal->exact);
        }
        if (ast->expr->kind == EXPR_CHR_LITERAL) {
            const nnc_chr_literal* literal = chr_literal(ast->expr);
            printf("val: [%d] -> %c\n", (nnc_i32)literal->exact, literal->exact);
        }
        if (ast->expr->kind == EXPR_STR_LITERAL) {
            const nnc_str_literal* literal = str_literal(ast->expr);
            printf("val: \"%s\"\n", literal->exact);
        }
        if (ast->expr->kind == EXPR_BINARY) {
            printf("binary: \n");
        }
        */
    }
    CATCH(NNC_SYNTAX) {
        nnc_abort_no_ctx(sformat("%s: %s (%s)\n", CATCHED.repr, CATCHED.what,
            nnc_ctx_tostr(CATCHED.data)));
    }
    CATCHALL {
        nnc_abort_no_ctx(sformat("%s: %s\n", CATCHED.repr, CATCHED.what));
    }
    
    /*
    nnc_parser parser = { 0 };
    TRY {
        nnc_parser_init(&parser, argv[1]);
        nnc_expression* expr = nnc_parse_expr(&parser);
        nnc_parser_fini(&parser);
    }
    CATCHALL {
        nnc_abort_no_ctx(sformat("%s: %s\n", CATCHED.repr, CATCHED.what));
    }
    */
    /*
    nnc_lex l = { 0 };
    TRY {
        nnc_lex_init(&l, argv[1]);
        for (;;) {
            nnc_tok_kind tok = nnc_lex_next(&l);
            printf("%s: %s", nnc_tok_str(tok), nnc_ctx_tostr(&l.cctx));
            if (tok == TOK_NUMBER) {
                printf(" -> [%s]", l.ctok.lexeme);
            }
            printf("\n");
            if (tok == TOK_EOF) {
                break;
            }
        }
        ETRY;
    }
    CATCH (NNC_LEX_BAD_FILE) {
        nnc_abort(sformat("%s: %s\n", CATCHED.repr,
            CATCHED.what), NULL);
    }
    */
    return EXIT_SUCCESS;
}

nnc_i32 main(nnc_i32 argc, char** argv) {
    nnc_reset_canarie();
    nnc_arena_init(&glob_arena);
    nnc_deferred_stack_init(&glob_deferred_stack);
    nnc_i32 status = nnc_main(argc, argv);
    nnc_arena_fini(&glob_arena);
    return status;
}