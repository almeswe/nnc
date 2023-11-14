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

nnc_static nnc_3a_cfg_node* nnc_3a_mknode(const nnc_3a_basic* block) {
    nnc_3a_cfg_node* node = anew(nnc_3a_cfg_node);
    node->id = block->id;
    node->block = block;
    assert(buf_len(block->quads) > 0);
    if (block->quads[0].label != 0) {
        node->labeled = true;
    }
    return node;
}

nnc_static nnc_3a_cfg_node* nnc_3a_get_labeled_node(nnc_u32 label, _vec_(nnc_3a_cfg_node*) nodes) {
    for (nnc_u64 i = 0; i < buf_len(nodes); i++) {
        if (nnc_3a_node_label(nodes[i]) == label) {
            return nodes[i];
        }
    }
    assert(false);
    return NULL;
}

nnc_static void nnc_3a_cfg_traverse_node(nnc_3a_cfg_node* node) {
    if (!node->unreachable) {
        return;
    }
    node->unreachable = false;
    if (node->next != NULL) {
        nnc_3a_cfg_traverse_node(node->next);
    }
    if (node->jump != NULL) {
        nnc_3a_cfg_traverse_node(node->jump);
    }
}

nnc_static nnc_3a_cfg* nnc_3a_cfg_traverse(nnc_3a_cfg* cfg) {
    nnc_3a_cfg_traverse_node(cfg->root);
    return cfg;
}

nnc_static nnc_3a_cfg* nnc_3a_cfg_restore(nnc_3a_cfg* cfg) {
    cfg->root = NULL;
    nnc_u64 cfg_nodes = buf_len(cfg->nodes);
    if (cfg_nodes != 0) {
        cfg->root = cfg->nodes[0];
    }
    for (nnc_u64 i = 0; i < cfg_nodes; i++) {
        cfg->nodes[i]->unreachable = true;
        cfg->nodes[i]->next = NULL;
        cfg->nodes[i]->jump = NULL;
        const nnc_3a_quad* last = NULL;
        const nnc_3a_basic* block = cfg->nodes[i]->block;
        nnc_bool last_node = i == cfg_nodes-1;
        if (buf_len(block->quads) != 0) {
            last = &buf_last(block->quads);
        }
        assert(last != NULL);
        if (last->op == OP_RETF || last->op == OP_RETP) {
            // `next` and `jump` pointers are already NULL
            continue;
        }
        if (last->op == OP_UJUMP) {
            // `next` pointer is already NULL
            nnc_u32 target = nnc_3a_jump_label(last);
            cfg->nodes[i]->jump = nnc_3a_get_labeled_node(target, cfg->nodes);
            continue;
        }
        if (nnc_3a_jump_op(last->op)) {
            cfg->nodes[i]->next = last_node ? NULL : cfg->nodes[i+1];
            nnc_u32 target = nnc_3a_jump_label(last);
            cfg->nodes[i]->jump = nnc_3a_get_labeled_node(target, cfg->nodes);
            continue;
        }
        if (last->label != 0 && last->op == OP_NONE) {
            // `jump` pointer is already NULL
            cfg->nodes[i]->next = last_node ? NULL : cfg->nodes[i+1];
            continue;
        }
    }
    cfg = nnc_3a_cfg_traverse(cfg);
    return cfg;
}

nnc_3a_cfg nnc_3a_cfg_optimize(nnc_3a_cfg cfg) {
    return cfg;
}

nnc_3a_cfg nnc_3a_get_cfg(const _vec_(nnc_3a_basic) blocks) {
    nnc_3a_cfg cfg = {
        .root = NULL,
        .nodes = NULL
    };
    for (nnc_u64 i = 0; i < buf_len(blocks); i++) {
        buf_add(cfg.nodes, nnc_3a_mknode(&blocks[i]));
    }
    nnc_3a_cfg_restore(&cfg);
    return cfg;
}