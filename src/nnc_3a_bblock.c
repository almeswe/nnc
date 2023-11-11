#include "nnc_3a.h"

nnc_static nnc_u32 block_cnt = 0;

nnc_static nnc_3a_bblock* nnc_3a_bblock_new() {
    nnc_3a_bblock* block = anew(nnc_3a_bblock);
    block->id = ++block_cnt;
    block->quads = NULL;
    return block;
}

nnc_static nnc_bool nnc_3a_jmp_op(nnc_3a_op_kind op) {
    switch (op) {
        case OP_UJUMP:
        case OP_CJUMPT: 
        case OP_CJUMPF:
        case OP_CJUMPE:
        case OP_CJUMPLT:
        case OP_CJUMPGT:
        case OP_CJUMPNE:
        case OP_CJUMPLTE:
        case OP_CJUMPGTE:
            return true;
        default: break;
    }
    return false;
}

nnc_3a_bblock* nnc_3a_basic_blocks(const nnc_3a_quad_set* set) {
    nnc_3a_bblock* block = NULL;
    nnc_3a_bblock* blocks = NULL;
    for (nnc_u64 i = 0; i < buf_len(set->quads); i++) {
        const nnc_3a_quad* quad = &set->quads[i];
            if (i == 0 || nnc_3a_jmp_op(quad->op) || quad->label != 0) {
                if (block != NULL) {
                    buf_add(blocks, *block);
                }
                block = nnc_3a_bblock_new();
            }
            buf_add(block->quads, *quad);
    }
    if (block != NULL && buf_len(block->quads) != 0) {
        //assert(buf_len(block->quads) == 0);
        buf_add(blocks, *block);
    }
    else {
        //todo: may be free here
    }
    return blocks;
}