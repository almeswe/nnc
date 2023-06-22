#include "nnc_ast.h"
#include "nnc_misc.h"

#define TREE_BR 	"\\_"
#define TREE_BV 	"| "
#define TREE_BVR    "|_"

#define DUMP_DATA(i, e) (nnc_dump_data) { .indent = i, .exact = (nnc_heap_ptr)(e) }

typedef struct _nnc_dump_data {
    nnc_i64 indent;
    nnc_heap_ptr exact;
} nnc_dump_data;

typedef void (*nnc_dump_fn)(nnc_dump_data);

static void nnc_dump_expr(nnc_dump_data data);

static nnc_bool nnc_is_escape(nnc_byte code) {
    switch (code) {
        case '\0': case '\a':
        case '\b': case '\f':
        case '\t': case '\v':
        case '\r': case '\n':
        case '\\': case '\'':
        case '\"':
            return true;
    }
    return false;
}

static nnc_byte nnc_escape(nnc_byte code) {
    switch (code) {
        case '\0': return '0';
        case '\a': return 'a';
        case '\b': return 'b';
        case '\f': return 'f';
        case '\t': return 't';
        case '\v': return 'v';
        case '\r': return 'r';
        case '\n': return 'n';
        case '\\': return '\\'; 
        case '\'': return '\'';
        case '\"': return '\"';
    }
    nnc_abort_no_ctx("nnc_get_escape_repr: unknown escape character.\n");
    return '\0';
}

static void nnc_dump_indent(nnc_i64 indent) {
    for (nnc_i64 i = 0; i < indent; i++) {
        fprintf(stderr, "%s", "  ");
    }
}

static void nnc_dump_type(const nnc_type* type) {
    if (type == NULL || type->kind == TYPE_UNKNOWN) {
        fprintf(stderr, _c(RED, "%s"), "?");
    }
    else {
        fprintf(stderr, _c(BBLU, "%s"), nnc_type_tostr(type));
    }
}

static void nnc_dump_chr(nnc_dump_data data) {
    const nnc_chr_literal* literal = data.exact;
    nnc_byte c_repr[3] = { 0 };
    c_repr[0] = literal->exact;
    if (nnc_is_escape(literal->exact)) {
        c_repr[0] = '\\';
        c_repr[1] = nnc_escape(literal->exact);
    }
    fprintf(stderr, _c(BYEL, "chr") " <val=\'%s\',code=%d>\n", 
        c_repr, (nnc_i32)literal->exact);
}

static void nnc_dump_str(nnc_dump_data data) {
    const nnc_str_literal* literal = data.exact;
    fprintf(stderr, _c(BYEL, "str") " <val=");
    for (nnc_u64 i = 0; i < literal->length; i++) {
        nnc_byte code = literal->exact[i];
        if (!nnc_is_escape(code)) {
            fprintf(stderr, "%c", code);
        }
        else {
            fprintf(stderr, "\\%c", nnc_escape(code));
        }
    }
    fprintf(stderr, ",len=%lu>\n", literal->length);
}

static void nnc_dump_ident(nnc_dump_data data) {
    const nnc_ident* ident = data.exact;
    fprintf(stderr, _c(BCYN, "ident "));
    fprintf(stderr, "<val=\"%s\",len=%lu>\n", ident->name, ident->size);
}

static void nnc_dump_int(nnc_dump_data data) {
    const nnc_int_literal* literal = data.exact;
    fprintf(stderr, _c(BYEL, "int") " <");
    if (literal->is_signed) {
        fprintf(stderr, "val=%ld,", literal->exact.d);
        fprintf(stderr, "signed,");
    }
    else {
        fprintf(stderr, "%lu", literal->exact.u);
    }
    fprintf(stderr, "base=%d,", literal->base);
    fprintf(stderr, "suff=%d>\n", literal->suffix);
}

static void nnc_dump_dbl(nnc_dump_data data) {
    const nnc_dbl_literal* literal = data.exact;
    fprintf(stderr, _c(BYEL, "float "));
    fprintf(stderr, "%f", literal->exact);
    fprintf(stderr, "<suff=%d>\n", literal->suffix);
}

static void nnc_dump_call(nnc_dump_data data) {
    const nnc_unary_expression* unary = data.exact;
    nnc_dump_expr(DUMP_DATA(data.indent + 1, unary->expr));
    nnc_dump_indent(data.indent + 2);
    fprintf(stderr, "%s", TREE_BR _c(BCYN, " args") "=\n");
    for (nnc_u64 i = 0; i < unary->exact.call.argc; i++) {
        const nnc_expression* arg = unary->exact.call.args[i];
        nnc_dump_expr(DUMP_DATA(data.indent + 3, arg));
    }
}

static void nnc_dump_unary(nnc_dump_data data) {
    const nnc_unary_expression* unary = data.exact;
    static const char* unary_str[] = {
        [UNARY_CAST]         = "cast",
        [UNARY_PLUS]         = "+",
        [UNARY_MINUS]        = "-",
        [UNARY_BITWISE_NOT]  = "~",
        [UNARY_DEREF]        = "*",
        [UNARY_REF]          = "&",
        [UNARY_NOT]          = "!",
        [UNARY_SIZEOF]       = "sizeof",
        [UNARY_LENGTHOF]     = "lengthof",
        [UNARY_POSTFIX_AS]   = "as",
        [UNARY_POSTFIX_CALL] = "()"
    };
    fprintf(stderr, _c(BGRN, "unary-expr `%s` "), unary_str[unary->kind]);
    if (unary->kind == UNARY_CAST   ||
        unary->kind == UNARY_SIZEOF ||
        unary->kind == UNARY_POSTFIX_AS) {
        const nnc_type* type = unary->kind == UNARY_SIZEOF ?
            unary->exact.size.of : unary->exact.cast.to;
        fprintf(stderr, "<type="); nnc_dump_type(type);
        fprintf(stderr, ">");
    }
    fprintf(stderr, "\n");
    nnc_dump_indent(data.indent + 1); fprintf(stderr, TREE_BR);
    nnc_dump_expr(DUMP_DATA(data.indent + 1, unary->expr));
    if (unary->kind == UNARY_POSTFIX_CALL) {
        for (nnc_u64 i = 0; i < unary->exact.call.argc; i++) {
            nnc_dump_indent(data.indent + 1);
            fprintf(stderr, TREE_BR "arg%lu=", i+1);
            nnc_dump_expr(DUMP_DATA(data.indent + 1, unary->exact.call.args[i]));
        }
    }
}

static void nnc_dump_binary(nnc_dump_data data) {
    const nnc_binary_expression* binary = data.exact;
    static const char* binary_str[] = {
        [BINARY_ADD]    = "+",
        [BINARY_SUB]    = "-",
        [BINARY_MUL]    = "*",
        [BINARY_DIV]    = "/",
        [BINARY_MOD]    = "%",
        [BINARY_SHR]    = ">>",
        [BINARY_SHL]    = "<<",
        [BINARY_LT]     = "<", 
        [BINARY_GT]     = ">",
        [BINARY_LTE]    = "<=",
        [BINARY_GTE]    = ">=",
        [BINARY_EQ]     = "==",
        [BINARY_NEQ]    = "==",
        [BINARY_BW_AND] = "&",
        [BINARY_BW_XOR] = "^",
        [BINARY_BW_OR]  = "|",
        [BINARY_AND]    = "&&",
        [BINARY_OR]     = "||",
        [BINARY_DOT]    = ".",
        [BINARY_IDX]    = "[]",
        [BINARY_COMMA]  = ","
    };
    fprintf(stderr, _c(BGRN, "binary-expr `%s`\n"), binary_str[binary->kind]);
    nnc_dump_indent(data.indent + 1); fprintf(stderr, TREE_BR);
    nnc_dump_expr(DUMP_DATA(data.indent + 1, binary->lexpr));
    nnc_dump_indent(data.indent + 1); fprintf(stderr, TREE_BR);
    nnc_dump_expr(DUMP_DATA(data.indent + 1, binary->rexpr));
}

static void nnc_dump_ternary(nnc_dump_data data) {
    const nnc_ternary_expression* ternary = data.exact;
    fprintf(stderr, _c(BGRN, "ternary-expr\n"));
    nnc_dump_indent(data.indent + 1); 
    fprintf(stderr, TREE_BR "condn=");
    nnc_dump_expr(DUMP_DATA(data.indent + 1, ternary->cexpr));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "lexpr=");
    nnc_dump_expr(DUMP_DATA(data.indent + 1, ternary->lexpr));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "rexpr=");
    nnc_dump_expr(DUMP_DATA(data.indent + 1, ternary->rexpr));
}

static void nnc_dump_expr(nnc_dump_data data) {
    const nnc_expression* expr = data.exact;
    static const nnc_dump_fn dumpers[] = {
        [EXPR_DBL_LITERAL] = nnc_dump_dbl,
        [EXPR_INT_LITERAL] = nnc_dump_int,
        [EXPR_CHR_LITERAL] = nnc_dump_chr,
        [EXPR_STR_LITERAL] = nnc_dump_str,
        [EXPR_IDENT]       = nnc_dump_ident,
        [EXPR_UNARY]       = nnc_dump_unary,
        [EXPR_BINARY]      = nnc_dump_binary,
        [EXPR_TERNARY]     = nnc_dump_ternary
    };
    if (expr != NULL) {
        dumpers[expr->kind](DUMP_DATA(data.indent, expr->exact));
    }
} 

static void nnc_dump_let_stmt(nnc_dump_data data) {
    const nnc_let_statement* let_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "let-stmt "));
    fprintf(stderr, "<var=%s,", let_stmt->var->name);
    fprintf(stderr, "type=");
    nnc_dump_type(let_stmt->type);
    fprintf(stderr, ">\n");
    if (let_stmt->init != NULL) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR "%s", "init-with=");
        nnc_dump_expr(DUMP_DATA(data.indent + 1, let_stmt->init));
    }
}

static void nnc_dump_type_stmt(nnc_dump_data data) {
    const nnc_type_statement* type_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "type-stmt ") "<type=");
    nnc_dump_type(type_stmt->type);
    fprintf(stderr, ",as=");
    nnc_dump_type(type_stmt->as);
    fprintf(stderr, ">\n");
}

static void nnc_dump_stmt(nnc_dump_data data) {
    const nnc_statement* stmt = data.exact;
    static const nnc_dump_fn dumpers[] = {
        [STMT_LET]  = nnc_dump_let_stmt,
        [STMT_TYPE] = nnc_dump_type_stmt
    };
    if (stmt != NULL) {
        dumpers[stmt->kind](DUMP_DATA(data.indent, stmt->exact));
    }
}

void nnc_dump_ast(const nnc_ast* ast) {
    fprintf(stderr, _c(BCYN, "%s\n"), "ast");
    nnc_dump_indent(1);
    nnc_dump_stmt(DUMP_DATA(1, ast->root));
    fflush(stderr);
}