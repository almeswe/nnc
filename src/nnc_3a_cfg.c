#include "nnc_3a.h"

nnc_static nnc_bool nnc_3a_jump_op(nnc_3a_op_kind op) {
    switch (op) {
        case OP_RETF: 
        case OP_RETP:     case OP_UJUMP:
        case OP_CJUMPT:   case OP_CJUMPF:
        case OP_CJUMPE:   case OP_CJUMPLT:
        case OP_CJUMPGT:  case OP_CJUMPNE:
        case OP_CJUMPLTE: case OP_CJUMPGTE:
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
        /*
            First quad is leader by default.
        */
        if (i == 0) {
            block = nnc_3a_mkblock(++block_id);
        }
        /*
            Labeled quad (or jump target) is leader.
        */
        if (quad->label != 0) {
            /*
                If current block is empty,
                add this quad there, no need
                to finish empty block.
            */
            if (buf_len(block.quads) == 0) {
                buf_add(block.quads, *quad);
            }
            /*
                Finish last block if it was not empty,
                and create new one with this quad added.
            */
            else {
                buf_add(blocks, block);
                block = nnc_3a_mkblock(++block_id);
                buf_add(block.quads, *quad);
            }
            /*
                But if current labeled quad
                is also jump operator, finish
                this block and create new empty one.
            */
            if (nnc_3a_jump_op(quad->op)) {
                buf_add(blocks, block);
                block = nnc_3a_mkblock(++block_id);
            }
            continue;
        }
        /*
            Jump operator quad is leader.
        */
        if (nnc_3a_jump_op(quad->op)) {
            buf_add(block.quads, *quad);
            buf_add(blocks, block);
            block = nnc_3a_mkblock(++block_id);
            continue;
        }
        buf_add(block.quads, *quad);
    }
    if (buf_len(block.quads) != 0) {
        buf_add(blocks, block);
    }
    return blocks;
}

nnc_static nnc_3a_cfg_node* nnc_3a_mknode(nnc_3a_basic* block) {
    nnc_3a_cfg_node* node = anew(nnc_3a_cfg_node);
    node->id = block->id;
    node->block = block;
    assert(buf_len(block->quads) > 0);
    if (block->quads[0].label != 0) {
        node->labeled = true;
    }
    return node;
}

nnc_static void nnc_3a_cfg_node_free(nnc_3a_cfg_node* node) {
    buf_free(node->block->quads);
    nnc_dispose(node->block);
    nnc_dispose(node);
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
    if (cfg != NULL && cfg->root != NULL) {
        nnc_3a_cfg_traverse_node(cfg->root);
    }
    return cfg;
}

nnc_static _map_(nnc_u32, nnc_3a_cfg_node*) nnc_3a_init_node_map(nnc_3a_cfg* cfg) {
    nnc_u64 cfg_nodes = buf_len(cfg->nodes);
    nnc_u64 node_map_cap = nnc_max(1, cfg_nodes / 2);
    _map_(nnc_u32, nnc_3a_cfg_node*) node_map = map_init_with(node_map_cap);
    for (nnc_u64 i = 0; i < cfg_nodes; i++) {
        const nnc_3a_basic* block = cfg->nodes[i]->block;
        assert(buf_len(block->quads) != 0);
        for (nnc_u64 j = 0; j < buf_len(block->quads); j++) {
            if (block->quads[j].label != 0) {
                map_put(node_map, block->quads[j].label, cfg->nodes[i]);
                break;
            }
        }
    }
    return node_map;
}

nnc_static nnc_3a_cfg* nnc_3a_cfg_rebuild(nnc_3a_cfg* cfg) {
    cfg->root = NULL;
    nnc_u64 cfg_nodes = buf_len(cfg->nodes);
    _map_(nnc_u32, nnc_3a_cfg_node*) node_map = NULL;
    if (cfg_nodes != 0) {
        cfg->root = cfg->nodes[0];
    }
    node_map = nnc_3a_init_node_map(cfg);
    for (nnc_u64 i = 0; i < cfg_nodes; i++) {
        cfg->nodes[i]->next = NULL;
        cfg->nodes[i]->jump = NULL;
        cfg->nodes[i]->unreachable = true;
        const nnc_3a_quad* last = NULL;
        const nnc_3a_basic* block = cfg->nodes[i]->block;
        nnc_bool last_node = i == cfg_nodes-1;
        if (buf_len(block->quads) != 0) {
            last = &buf_last(block->quads);
        }
        assert(last != NULL);
        /*
            When basic block contains return operator, this block
            will not yield any relations with other blocks further.
        */
        if (last->op == OP_RETF || 
            last->op == OP_RETP) {
            continue;
        }
        /*
            In case of conditional operator, both `jump` and `next`
            branches are set. `next` branch will be NULL in case of
            unconditional jump.
        */
        if (nnc_3a_jump_op(last->op)) {
            nnc_u32 jump_to = nnc_3a_jump_label(last);
            assert(map_has(node_map, jump_to));
            cfg->nodes[i]->jump = map_get(node_map, jump_to);
            if (last->op != OP_UJUMP) {
                cfg->nodes[i]->next = last_node ? 
                    NULL : cfg->nodes[i+1];
            }
            continue;
        }
        /*
            In any other case just connect current (i)
            node with further (i+1) node via `next` branch.
        */
        cfg->nodes[i]->next = last_node ? 
            NULL : cfg->nodes[i+1];
    }
    map_fini(node_map);
    nnc_3a_cfg_traverse(cfg);
    return cfg;
}

nnc_3a_cfg nnc_3a_cfg_optimize(nnc_3a_cfg cfg, nnc_3a_opt_stat* stat) {
    nnc_u64 reduced = 0;
    _vec_(nnc_3a_cfg_node*) used_nodes = NULL;
    for (nnc_u64 i = 0; i < buf_len(cfg.nodes); i++) {
        if (cfg.nodes[i]->unreachable) {
            reduced += buf_len(cfg.nodes[i]->block->quads);
            nnc_3a_cfg_node_free(cfg.nodes[i]);
        }
        else {
            buf_add(used_nodes, cfg.nodes[i]);
        }
    }
    if (stat != NULL) {
        stat->reduced += reduced;
        stat->percent = stat->reduced / (nnc_f32)stat->initial;
        if (stat->initial == 0) {
            stat->percent = 0.0;
        }
    }
    buf_free(cfg.nodes);
    cfg.nodes = used_nodes;
    nnc_3a_cfg_rebuild(&cfg);
    return cfg;
}

nnc_3a_cfg nnc_3a_get_cfg(_vec_(nnc_3a_basic) blocks) {
    nnc_3a_cfg cfg = {
        .root = NULL,
        .nodes = NULL
    };
    for (nnc_u64 i = 0; i < buf_len(blocks); i++) {
        buf_add(cfg.nodes, nnc_3a_mknode(&blocks[i]));
    }
    nnc_3a_cfg_rebuild(&cfg);
    return cfg;
}