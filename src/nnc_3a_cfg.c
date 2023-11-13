#include "nnc_3a.h"

nnc_static nnc_bool nnc_3a_jump_op(nnc_3a_op_kind op) {
    switch (op) {
        case OP_RETF:
        case OP_RETP:
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

_vec_(nnc_3a_basic) nnc_3a_get_blocks(const nnc_3a_quad_set* set) {
    nnc_u32 block_id = 0;
    nnc_u64 size = buf_len(set->quads);
    nnc_3a_basic block = {0};
    _vec_(nnc_3a_basic) blocks = NULL;
    for (nnc_u64 i = 0; i < size; i++) {
        const nnc_3a_quad* quad = &set->quads[i];
        if (i == 0) {
            block = nnc_3a_mkblock(++block_id);
        }
        if (nnc_3a_jump_op(quad->op) || 
           (quad->label != 0 && buf_len(block.quads) == 0)) {
            buf_add(block.quads, *quad);
            buf_add(blocks, block);
            block = nnc_3a_mkblock(++block_id);
            continue;
        }
        if (quad->label != 0) {
            buf_add(blocks, block);
            block = nnc_3a_mkblock(++block_id);
            buf_add(block.quads, *quad);
            continue;
        }
        if (i == size-1) {
            buf_add(block.quads, *quad);
            buf_add(blocks, block);
            continue;
        }
        buf_add(block.quads, *quad);
    }
    return blocks;
}