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
static void nnc_dump_stmt(nnc_dump_data data);
static void nnc_dump_compound_stmt(nnc_dump_data data);

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
        fprintf(stderr, "val=%lu,", literal->exact.u);
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
        [BINARY_ADD]        = "+",
        [BINARY_SUB]        = "-",
        [BINARY_MUL]        = "*",
        [BINARY_DIV]        = "/",
        [BINARY_MOD]        = "%",
        [BINARY_SHR]        = ">>",
        [BINARY_SHL]        = "<<",
        [BINARY_LT]         = "<", 
        [BINARY_GT]         = ">",
        [BINARY_LTE]        = "<=",
        [BINARY_GTE]        = ">=",
        [BINARY_EQ]         = "==",
        [BINARY_NEQ]        = "==",
        [BINARY_BW_AND]     = "&",
        [BINARY_BW_XOR]     = "^",
        [BINARY_BW_OR]      = "|",
        [BINARY_AND]        = "&&",
        [BINARY_OR]         = "||",
        [BINARY_DOT]        = ".",
        [BINARY_IDX]        = "[]",
        [BINARY_ASSIGN] = "=",
        [BINARY_COMMA]      = ","
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

static void nnc_dump_if_stmt(nnc_dump_data data) {
    const nnc_if_stmt* if_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "if-stmt\n"));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "if-cond=");
    nnc_dump_expr(DUMP_DATA(data.indent+1, if_stmt->if_br->cond));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "if-body=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, if_stmt->if_br->body));
    for (nnc_u64 i = 0; i < buf_len(if_stmt->elif_brs); i++) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR _c(BMAG, "elif%lu-stmt\n"), i+1);
        nnc_dump_indent(data.indent + 2);
        fprintf(stderr, TREE_BR "elif-cond=");
        nnc_dump_expr(DUMP_DATA(data.indent+2, if_stmt->elif_brs[i]->cond));
        nnc_dump_indent(data.indent + 2);
        fprintf(stderr, TREE_BR "elif-body=");
        nnc_dump_stmt(DUMP_DATA(data.indent+2, if_stmt->elif_brs[i]->body));
    }
    if (if_stmt->else_br != NULL) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR _c(BMAG, "else-stmt\n"));
        nnc_dump_indent(data.indent + 2);
        fprintf(stderr, TREE_BR "else-body=");
        nnc_dump_stmt(DUMP_DATA(data.indent+2, if_stmt->else_br));
    }
}

static void nnc_dump_do_stmt(nnc_dump_data data) {
    const nnc_do_while_stmt* do_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "do-stmt\n"));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "body=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, do_stmt->body));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "cond=");
    nnc_dump_expr(DUMP_DATA(data.indent+1, do_stmt->cond));
}

static void nnc_dump_for_stmt(nnc_dump_data data) {
    const nnc_for_stmt* for_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "for-stmt\n"));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "init=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, for_stmt->init));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "cond=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, for_stmt->cond));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "step=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, for_stmt->step));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "body=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, for_stmt->body));
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

static void nnc_dump_goto_stmt(nnc_dump_data data) {
    const nnc_goto_statement* goto_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "goto-stmt ") "<label=");
    const nnc_expression_statement* body = goto_stmt->body->exact;
    nnc_dump_expr(DUMP_DATA(data.indent+1, body->expr));
}

static void nnc_dump_type_stmt(nnc_dump_data data) {
    const nnc_type_statement* type_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "type-stmt ") "<type=");
    nnc_dump_type(type_stmt->type);
    fprintf(stderr, ",as=");
    nnc_dump_type(type_stmt->as);
    fprintf(stderr, ">\n");
}

static void nnc_dump_while_stmt(nnc_dump_data data) {
    const nnc_while_stmt* while_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "while-stmt\n"));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "cond=");
    nnc_dump_expr(DUMP_DATA(data.indent+1, while_stmt->cond));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "body=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, while_stmt->body));
}

static void nnc_dump_empty_stmt(nnc_dump_data data) {
    fprintf(stderr, _c(BMAG, "empty-stmt\n"));
}

static void nnc_dump_break_stmt(nnc_dump_data data) {
    fprintf(stderr, _c(BMAG, "break-stmt\n"));
}

static void nnc_dump_return_stmt(nnc_dump_data data) {
    const nnc_return_statement* ret_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "return-stmt\n"));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "what=");
    nnc_dump_stmt(DUMP_DATA(data.indent+1, ret_stmt->body));
}

static void nnc_dump_expr_stmt(nnc_dump_data data) {
    const nnc_expression_statement* expr_stmt = data.exact;
    fprintf(stderr, _c(BMAG, "expr-stmt\n"));
    nnc_dump_indent(data.indent + 1);
    fprintf(stderr, TREE_BR "expr=");
    nnc_dump_expr(DUMP_DATA(data.indent+1, expr_stmt->expr));
}

static void nnc_dump_compound_stmt(nnc_dump_data data) {
    const nnc_compound_statement* compound = data.exact;
    fprintf(stderr, _c(BMAG, "compound-stmt"));
    fprintf(stderr, " <stmts=%lu>\n", buf_len(compound->stmts));
    for (nnc_u64 i = 0; i < buf_len(compound->stmts); i++) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR "<stmt%lu>=", i+1);
        nnc_dump_stmt(DUMP_DATA(data.indent+1, compound->stmts[i]));
    }
}

static void nnc_dump_continue_stmt(nnc_dump_data data) {
    fprintf(stderr, _c(BMAG, "continue-stmt\n"));
}

static void nnc_dump_namespace_stmt(nnc_dump_data data) {
    const nnc_namespace_statement* namespace = data.exact;
    fprintf(stderr, _c(BRED, "namespace-stmt ") "<name=%s>\n", namespace->var->name);
    for (nnc_u64 i = 0; i < buf_len(namespace->stmts); i++) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR "<topmost stmt%lu>=", i+1);
        nnc_dump_stmt(DUMP_DATA(data.indent+1, namespace->stmts[i]));
    }
}

static void nnc_dump_fn_stmt(nnc_dump_data data) {
    const nnc_fn_statement* fn_stmt = data.exact;
    fprintf(stderr, _c(BRED, "fn-stmt ") "<name=%s, paramc=%lu, ret-type=",
        fn_stmt->var->name, buf_len(fn_stmt->params));
    nnc_dump_type(fn_stmt->ret);
    fprintf(stderr, ">\n");
    for (nnc_u64 i = 0; i < buf_len(fn_stmt->params); i++) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR "<param%lu>=", i+1);
        fprintf(stderr, "%s: ", fn_stmt->params[i]->var->name);
        nnc_dump_type(fn_stmt->params[i]->type);
        fprintf(stderr, "\n");
    }
    for (nnc_u64 i = 0; i < buf_len(fn_stmt->body); i++) {
        nnc_dump_indent(data.indent + 1);
        fprintf(stderr, TREE_BR "<stmt%lu>=", i+1);
        nnc_dump_stmt(DUMP_DATA(data.indent+1, fn_stmt->body[i]));
    }
}

static void nnc_dump_stmt(nnc_dump_data data) {
    const nnc_statement* stmt = data.exact;
    static const nnc_dump_fn dumpers[] = {
        [STMT_IF]        = nnc_dump_if_stmt,
        [STMT_DO]        = nnc_dump_do_stmt,
        [STMT_FOR]       = nnc_dump_for_stmt,
        [STMT_LET]       = nnc_dump_let_stmt,
        [STMT_GOTO]      = nnc_dump_goto_stmt,
        [STMT_TYPE]      = nnc_dump_type_stmt,
        [STMT_EXPR]      = nnc_dump_expr_stmt,
        [STMT_WHILE]     = nnc_dump_while_stmt,
        [STMT_EMPTY]     = nnc_dump_empty_stmt,
        [STMT_BREAK]     = nnc_dump_break_stmt,
        [STMT_RETURN]    = nnc_dump_return_stmt,
        [STMT_COMPOUND]  = nnc_dump_compound_stmt,
        [STMT_CONTINUE]  = nnc_dump_continue_stmt,
        [STMT_NAMESPACE] = nnc_dump_namespace_stmt,
        [STMT_FUNC_DECL] = nnc_dump_fn_stmt
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