#include "nnc_3a.h"
#include "nnc_gen.h"
#include "nnc_argv.h"

#define dump_indent "   "

#define REDIRECT_3a_DUMP_TO_BLOB 0

#if REDIRECT_3a_DUMP_TO_BLOB == 0
    #define dump_3a(fmt, ...) fprintf(stream, fmt, __VA_ARGS__)
#else
    #define dump_3a(...) text_put(__VA_ARGS__) 
#endif

static FILE* stream = NULL;
static char internal_buf[128] = {0};

//todo: move to separate file
extern nnc_bool nnc_is_escape(nnc_byte code);
extern nnc_byte nnc_escape(nnc_byte code);

static const char* op_str[] = {
    /*  binary operators */
    [OP_ADD]    = "+",
    [OP_SUB]    = "-",
    [OP_MUL]    = "*",
    [OP_DIV]    = "/",
    [OP_MOD]    = "%", 
    [OP_SHR]    = ">>",
    [OP_SHL]    = "<<",
    [OP_SAL]    = "sal",
    [OP_SAR]    = "sar",
    [OP_BW_AND] = "&",
    [OP_BW_XOR] = "^",
    [OP_BW_OR]  = "|",
    /*  unary operators  */
    [OP_PLUS]   = "+",
    [OP_MINUS]  = "-",
    [OP_BW_NOT] = "~"
    /* ***************** */
};

nnc_static const char* nnc_dump_3a_op(nnc_3a_op_kind op) {
    return op_str[op];
}

nnc_static void nnc_dump_3a_cgt(const nnc_3a_addr* addr) {
    memset(internal_buf, 0, sizeof internal_buf);
    sprintf(internal_buf, "t%lu", addr->exact.cgt);
}

nnc_static void nnc_dump_3a_none(const nnc_3a_addr* addr) {
    memset(internal_buf, 0, sizeof internal_buf);
}

nnc_static void nnc_dump_3a_name(const nnc_3a_addr* addr) {
    memset(internal_buf, 0, sizeof internal_buf);
    sprintf(internal_buf, "_%s", addr->exact.name.name);
}

nnc_static void nnc_dump_3a_sconst(const nnc_3a_addr* addr) {
    memset(internal_buf, 0, sizeof internal_buf);
    const nnc_str str = addr->exact.sconst.sconst;
    const nnc_u64 len = strlen(str);
    internal_buf[0] = '\"';
    for (nnc_u64 i = 0; i < len; i++) {
        if (nnc_is_escape(str[i])) {
            internal_buf[i + 1] = nnc_escape(str[i]);
        }
        else {
            internal_buf[i + 1] = str[i];
        }
    }
    internal_buf[len + 1] = '\"';
}

nnc_static void nnc_dump_3a_iconst(const nnc_3a_addr* addr) {
    memset(internal_buf, 0, sizeof internal_buf);
    const nnc_3a_iconst* iconst = &addr->exact.iconst;
    if (nnc_signed_type(addr->type)) {
        sprintf(internal_buf, "0x%lx", iconst->sconst);
    }
    else {
        sprintf(internal_buf, "0x%lx", iconst->iconst);
    }
}

nnc_static void nnc_dump_3a_fconst(const nnc_3a_addr* addr) {
    memset(internal_buf, 0, sizeof internal_buf);
    const nnc_3a_fconst* fconst = &addr->exact.fconst;
    sprintf(internal_buf, "%f", fconst->fconst);
}

nnc_static char* nnc_dump_3a_type(const nnc_type* type) {
    memset(internal_buf, 0, sizeof internal_buf);
    sprintf(internal_buf, "(%s)", nnc_type_tostr(type));
    return internal_buf;
}

nnc_static const char* nnc_dump_3a_addr(const nnc_3a_addr* addr) {
    switch (addr->kind) {
        case ADDR_CGT:    nnc_dump_3a_cgt(addr);    break;
        case ADDR_NONE:   nnc_dump_3a_none(addr);   break;
        case ADDR_NAME:   nnc_dump_3a_name(addr);   break;
        case ADDR_SCONST: nnc_dump_3a_sconst(addr); break;
        case ADDR_ICONST: nnc_dump_3a_iconst(addr); break;
        case ADDR_FCONST: nnc_dump_3a_fconst(addr); break;
    }
    return internal_buf;
}

nnc_static void nnc_dump_3a_binary(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_op(quad->op));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_unary(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_op(quad->op));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_arg(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    arg %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_ref(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" &%s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_copy(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_cast(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s", nnc_dump_3a_type(quad->res.type));
    dump_3a("%s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_retf(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%7s %s\n", "ret", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_retp(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%7s\n", "ret");
}

nnc_static void nnc_dump_3a_deref(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s =", nnc_dump_3a_addr(&quad->res));
    dump_3a(" *%s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_fcall(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%6s = call", nnc_dump_3a_addr(&quad->res));
    dump_3a(" @%s,", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_pcall(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    call @%s,", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg2));
}

nnc_static void nnc_dump_3a_ujump(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpt(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpf(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if not %s", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpe(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s ==", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumplt(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s <", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpgt(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s >", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpne(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s !=", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumplte(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s <=", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_cjumpgte(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "    if %s >=", nnc_dump_3a_addr(&quad->arg1));
    dump_3a(" %s", nnc_dump_3a_addr(&quad->arg2));
    dump_3a(" goto @%s\n", nnc_dump_3a_addr(&quad->res));
}

nnc_static void nnc_dump_3a_deref_copy(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%4s%s =", "*", nnc_dump_3a_addr(&quad->res));
    dump_3a(" %s\n", nnc_dump_3a_addr(&quad->arg1));
}

nnc_static void nnc_dump_3a_declare_local(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%s\n", "(declaration of local)");
}

nnc_static void nnc_dump_3a_declare_global(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%s\n", "(declaration of global)");
}

nnc_static void nnc_dump_3a_declare_string(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%s\n", "(declaration of string)");
}

nnc_static void nnc_dump_3a_prepare_call(const nnc_3a_quad* quad) {
    dump_3a(dump_indent "%s\n", "(preparation before call)");
}

nnc_static void nnc_dump_3a_quad_type(const nnc_3a_quad* quad) {
    static nnc_i32 pad = 10;
    const char* repr = "";
    if (quad->op < OP_UJUMP || quad->op > OP_CJUMPNE) {
        repr = quad->res.type == NULL ? 
            "" : nnc_dump_3a_type(quad->res.type); 
    }
    pad = nnc_max((int)strlen(repr), pad);
    dump_3a("%*s", pad, repr);
}

void nnc_dump_3a_quad(const nnc_3a_quad* quad) {
    if (stream == NULL) {
        stream = stderr;
    }
    if (quad->label != 0) {
        dump_3a(" @%u:\n", quad->label);
        if (quad->op == OP_NONE) {
            return;
        } 
    }
    nnc_dump_3a_quad_type(quad);
    switch (quad->op) {
        case OP_UNARY:        nnc_dump_3a_unary(quad);        break;
        case OP_BINARY:       nnc_dump_3a_binary(quad);       break;
        case OP_ARG:          nnc_dump_3a_arg(quad);          break;
        case OP_REF:          nnc_dump_3a_ref(quad);          break;
        case OP_COPY:         nnc_dump_3a_copy(quad);         break;
        case OP_CAST:         nnc_dump_3a_cast(quad);         break;
        case OP_RETF:         nnc_dump_3a_retf(quad);         break;
        case OP_RETP:         nnc_dump_3a_retp(quad);         break;
        case OP_DEREF:        nnc_dump_3a_deref(quad);        break;
        case OP_FCALL:        nnc_dump_3a_fcall(quad);        break;
        case OP_PCALL:        nnc_dump_3a_pcall(quad);        break;
        case OP_UJUMP:        nnc_dump_3a_ujump(quad);        break;
        case OP_CJUMPT:       nnc_dump_3a_cjumpt(quad);       break;
        case OP_CJUMPF:       nnc_dump_3a_cjumpf(quad);       break;
        case OP_CJUMPE:       nnc_dump_3a_cjumpe(quad);       break;
        case OP_CJUMPLT:      nnc_dump_3a_cjumplt(quad);      break;
        case OP_CJUMPGT:      nnc_dump_3a_cjumpgt(quad);      break;
        case OP_CJUMPNE:      nnc_dump_3a_cjumpne(quad);      break;
        case OP_CJUMPLTE:     nnc_dump_3a_cjumplte(quad);     break;
        case OP_CJUMPGTE:     nnc_dump_3a_cjumpgte(quad);     break;
        case OP_DEREF_COPY:   nnc_dump_3a_deref_copy(quad);   break;
        case OP_HINT_DECL_LOCAL:   nnc_dump_3a_declare_local(quad);  break;
        case OP_HINT_DECL_GLOBAL:  nnc_dump_3a_declare_global(quad); break;
        case OP_HINT_DECL_STRING:  nnc_dump_3a_declare_string(quad); break;
        case OP_HINT_PREPARE_CALL: nnc_dump_3a_prepare_call(quad);   break;
        default: { 
            nnc_abort_no_ctx("nnc_dump_3a_quad: unimplemented case met.\n");
        }
    }
}

void nnc_dump_3a_quads(const nnc_3a_quad* quads) {
    static nnc_u32 quad_counter = 0;
    for (nnc_u64 i = 0; i < buf_len(quads); i++) {
        fprintf(stream, "%3u) ", ++quad_counter);
        nnc_dump_3a_quad(&quads[i]);
    }
}

void nnc_dump_3a_cfg(const char* name, const nnc_3a_cfg* cfg) {
    dump_3a("%s:\n", name);
    for (nnc_u64 i = 0; i < buf_len(cfg->nodes); i++) {
        const nnc_3a_cfg_node* node = cfg->nodes[i]; 
        dump_3a("   B%u", node->id);
        if (node->unreachable) {
            dump_3a("%s", "(u)");
        }
        dump_3a("%s", "\n");
        if (node->jump != NULL) {
            dump_3a("    \\_ jump B%u\n", node->jump->id);
        }
        if (node->next != NULL) {
            dump_3a("    \\_ next B%u", node->next->id);
        }
        dump_3a("%s", "\n");
    }
}

nnc_static void nnc_dump_3a_lr_cgt(nnc_map_key key, nnc_map_val val) {
    const nnc_3a_lr* lr = (nnc_3a_lr*)val;
    fprintf(stream, "[t%u] lives at (%u:%u)\n", (nnc_u32)key, lr->starts+1, lr->ends+1);
}

nnc_static void nnc_dump_3a_lr_var(nnc_map_key key, nnc_map_val val) {
    const nnc_3a_lr* lr = (nnc_3a_lr*)val;
    fprintf(stream, "[_%s] lives at (%u:%u)\n", (char*)key, lr->starts+1, lr->ends+1);
}

nnc_static void nnc_dump_3a_var(const nnc_ir_var* var) {
    dump_3a("\n@v_%s: (%lu=%d%%)\n", var->name, var->stat.reduced,
        (nnc_i32)(var->stat.percent * 100));
    nnc_dump_3a_quads(var->quads);
}

void nnc_dump_3a_proc(const nnc_3a_proc* unit) {
    dump_3a("\n@_%s: (%lu=%d%%)\n", unit->name, unit->stat.reduced,
        (nnc_i32)(unit->stat.percent * 100));
    nnc_dump_3a_quads(unit->quads);
}

void nnc_dump_3a_code(const nnc_3a_code code) {
    stream = glob_argv.dump_dest;
    for (nnc_u64 i = 0; i < buf_len(code); i++) {
        nnc_dump_3a_proc(&code[i]);
    }
}

void nnc_dump_ir(const vector(nnc_ir_glob_sym) ir) {
    stream = glob_argv.dump_dest;
    for (nnc_u64 i = 0; i < buf_len(ir); i++) {
        switch (ir[i].kind) {
            case STMT_LET: nnc_dump_3a_var(&ir[i].sym.var);   break;
            case STMT_FN:  nnc_dump_3a_proc(&ir[i].sym.proc); break;
        }
    }
}