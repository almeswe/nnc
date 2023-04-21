#include <stdio.h>
#include "nnc_core.h"

nnc_arena glob_arena;

void nnc_arena_test() {
    int* heap_array[16];
    for (int i = 0; i < 8; i++) {
        heap_array[i] = nnc_alloc(sizeof(int));
        *heap_array[i] = i;
    }
    nnc_dispose(heap_array[0]);
    nnc_dispose(heap_array[2]);
    nnc_dispose(heap_array[3]);
    nnc_dispose(heap_array[4]);
    nnc_alloc(sizeof(int));
}

void nnc_buf_test() {
    int* int_buf = NULL;
    for (int i = 0; i < 2048; i++) {
        buf_add(int_buf, i);
    }
    for (int i = 0; i < buf_len(int_buf); i++) {
        printf("%d) %d\n", i+1, i);
    }
    printf("len  : %ld\n", buf_len(int_buf));
    printf("cap  : %ld\n", buf_cap(int_buf));
    printf("bytes: %ld\n", glob_arena.alloc_bytes);
}

void nnc_lex_test(char** argv) {
    nnc_lex lex = { 0 };
    lex.fpath = argv[1];
    nnc_lex_init(&lex);
    while (nnc_lex_next(&lex) != TOK_EOF) {
        printf("%s", nnc_tok_str(lex.ctok.kind));
        if (lex.ctok.kind == TOK_IDENT ||
            lex.ctok.kind == TOK_STR   ||
            lex.ctok.kind == TOK_CHR   ||
            lex.ctok.kind == TOK_NUMBER) {
            printf(" %s", lex.ctok.lexeme);
        }
        printf("\n");
    }
    nnc_lex_fini(&lex);
}

int main(int argc, char** argv) {
    INSIDE_ARENA(nnc_lex_test(argv));
    return EXIT_SUCCESS;
}