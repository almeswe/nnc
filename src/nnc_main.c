#include <stdio.h>
#include "nnc_core.h"

nnc_arena glob_arena;

void nnc_arena_test() {
    nnc_arena_init(&glob_arena);
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
    nnc_arena_fini(&glob_arena);
}

void nnc_buf_test() {
    int* int_buf = NULL;
    for (int i = 0; i < 128; i++) {
        buf_add(int_buf, i);
    }
    for (int i = 0; i < buf_len(int_buf); i++) {
        printf("%d) %d\n", i+1, i);
    }
    printf("len: %ld\n", buf_len(int_buf));
    printf("cap: %ld\n", buf_cap(int_buf));
    buf_free(int_buf);
}

int main(int argc, char** argv) {
    nnc_buf_test();
    return EXIT_SUCCESS;
}