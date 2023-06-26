#include "nnc_parse.h"

void nnc_parser_init(nnc_parser* out_parser, const char* file) {
    nnc_lex_init(&out_parser->lex, file);
    out_parser->lookup.is_first = true;
}

void nnc_parser_fini(nnc_parser* parser) {
    if (parser != NULL) {
        nnc_lex_fini(&parser->lex);
    }
}

nnc_tok* nnc_parser_get(nnc_parser* parser) {
    return &parser->current.tok;
}

nnc_tok* nnc_parser_get_lookup(nnc_parser* parser) {
    return &parser->lookup.tok;
}

nnc_tok_kind nnc_parser_peek(nnc_parser* parser) {
    return nnc_parser_get(parser)->kind;
}

nnc_tok_kind nnc_parser_peek_lookup(nnc_parser* parser) {
    return nnc_parser_get_lookup(parser)->kind;
}

nnc_bool nnc_parser_match(nnc_parser* parser, nnc_tok_kind kind) {
    return nnc_parser_peek(parser) == kind;
}

nnc_tok_kind nnc_parser_next(nnc_parser* parser) {
    nnc_lex_next(&parser->lex);
    if (!parser->lookup.is_first) {
        parser->current.tok = parser->lookup.tok;
    }
    else {
        parser->lookup.is_first = false;
        parser->current.tok = parser->lex.ctok;
        nnc_lex_next(&parser->lex);
    }
    parser->lookup.tok = parser->lex.ctok;
    return parser->current.tok.kind;
}

nnc_tok_kind nnc_parser_expect(nnc_parser* parser, nnc_tok_kind kind) {
    nnc_tok_kind current = nnc_parser_peek(parser);
    if (current != kind) {
        // todo: pass ctx to exception
        THROW(NNC_SYNTAX, sformat("expected <%s>, but met <%s>.", 
            nnc_tok_str(kind), nnc_tok_str(current)));
    }   
    nnc_parser_next(parser);
    return nnc_parser_peek(parser);
}

static nnc_bool nnc_parser_match_type(nnc_tok_kind kind) {
    switch (kind) {
        case TOK_VOID:
        case TOK_I8:  case TOK_I16:
        case TOK_I32: case TOK_I64:
        case TOK_U8:  case TOK_U16:
        case TOK_U32: case TOK_U64:
        case TOK_F32: case TOK_F64:
            return true;
        default:
            return false;
    }
}

static nnc_type* nnc_parse_type(nnc_parser* parser);

static nnc_type* nnc_parse_fn_type(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_FN);
    nnc_type* type = nnc_fn_type_new();
    nnc_parser_expect(parser, TOK_OPAREN);
    while (!nnc_parser_match(parser, TOK_CPAREN) &&
           !nnc_parser_match(parser, TOK_EOF)) {
        buf_add(type->exact.fn.params, nnc_parse_type(parser));
        if (nnc_parser_match(parser, TOK_CPAREN)) {
            break;
        }
        nnc_parser_expect(parser, TOK_COMMA);
    }
    type->exact.fn.paramc = buf_len(type->exact.fn.params);
    nnc_parser_expect(parser, TOK_CPAREN);
    nnc_parser_expect(parser, TOK_COLON);
    type->exact.fn.ret = nnc_parse_type(parser);
    return type;
}

static nnc_type* nnc_parse_user_type(nnc_parser* parser) {
    nnc_tok* tok = nnc_parser_get(parser);
    nnc_ident* ident = nnc_ident_new(tok->lexeme);
    nnc_parser_expect(parser, TOK_IDENT);
    return nnc_type_new(ident->name);
}

static nnc_type* nnc_parse_type_declarators(nnc_parser* parser, nnc_type* type) {
    while (nnc_parser_match(parser, TOK_ASTERISK) ||
           nnc_parser_match(parser, TOK_OBRACKET)) {
        nnc_tok_kind kind = nnc_parser_peek(parser);
        nnc_parser_next(parser);
        if (kind == TOK_ASTERISK) {
            type = nnc_ptr_type_new(type);
            if (type->base->kind == TYPE_ARRAY) {
                THROW(NNC_SYNTAX, sformat("cannot declare type \'%s\'.\n", nnc_type_tostr(type)));
            }
        }
        else {
            type = nnc_arr_type_new(type);
            type->exact.array.dim = nnc_parse_expr_reduced(parser);
            nnc_parser_expect(parser, TOK_CBRACKET);
        }
    }
    return type;
}

static nnc_type* nnc_parse_type(nnc_parser* parser) {
    nnc_type* type = NULL;
    const nnc_tok_kind kind = nnc_parser_peek(parser);
    switch (kind) {
        case TOK_I8:    type = &i8_type;   break;
        case TOK_U8:    type = &u8_type;   break;
        case TOK_I16:   type = &i16_type;  break;
        case TOK_U16:   type = &u16_type;  break;
        case TOK_I32:   type = &i32_type;  break;
        case TOK_U32:   type = &u32_type;  break;
        case TOK_F32:   type = &f32_type;  break;
        case TOK_I64:   type = &i64_type;  break;
        case TOK_U64:   type = &u64_type;  break;
        case TOK_F64:   type = &f64_type;  break;
        case TOK_VOID:  type = &void_type; break;
        case TOK_FN:    type = nnc_parse_fn_type(parser);   break;
        case TOK_IDENT: type = nnc_parse_user_type(parser); break;
        default: 
            nnc_abort_no_ctx("nnc_parse_type: unknown type kind met.");
    }
    if (kind != TOK_FN &&
        kind != TOK_IDENT) {
        nnc_parser_next(parser);
    }
    return nnc_parse_type_declarators(parser, type);
}

static nnc_expression* nnc_parse_dbl(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_dbl_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_DBL_LITERAL, exact);
}

static nnc_expression* nnc_parse_int(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_int_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_INT_LITERAL, exact);
}

static nnc_expression* nnc_parse_chr(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_chr_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_CHR_LITERAL, exact);
}

static nnc_expression* nnc_parse_str(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_str_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_STR_LITERAL, exact);
}

static nnc_expression* nnc_parse_ident(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_heap_ptr exact = nnc_ident_new(tok->lexeme);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_IDENT, exact);
}

static nnc_expression* nnc_parse_number(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    if (strchr(tok->lexeme, '.') != NULL) {
        return nnc_parse_dbl(parser);
    }
    return nnc_parse_int(parser);
}

static nnc_expression* nnc_parse_parens(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_expression* expr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    return expr;
}

static nnc_expression* nnc_parse_primary_expr(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_STR:    return nnc_parse_str(parser);
        case TOK_CHR:    return nnc_parse_chr(parser);
        case TOK_IDENT:  return nnc_parse_ident(parser);
        case TOK_NUMBER: return nnc_parse_number(parser);
        case TOK_OPAREN: return nnc_parse_parens(parser);
        default:
            THROW(NNC_UNINPLEMENTED, sformat("nnc_parse_expr -> %s.\n", 
                nnc_tok_str(tok->kind)));
    }
    return NULL;
}

static nnc_expression* nnc_parse_unary_expr(nnc_parser* parser);

static nnc_expression* nnc_parse_cast_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_OPAREN);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_CAST);
    expr->exact.cast.to = nnc_parse_type(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_plus_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_PLUS);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_minus_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_MINUS);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_bitwise_not_expr(nnc_parser* parser) {
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_BITWISE_NOT);
    nnc_parser_next(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_dereference_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_DEREF);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_reference_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_REF);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_not_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_NOT);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

static nnc_expression* nnc_parse_sizeof_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_SIZEOF);
    nnc_parser_expect(parser, TOK_OPAREN);
    expr->exact.size.of = nnc_parse_type(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_lengthof_expr(nnc_parser* parser) {
    nnc_parser_next(parser);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_LENGTHOF);
    expr->expr = nnc_parse_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_as_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_AS);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_AS);
    expr->expr = prefix;
    expr->exact.cast.to = nnc_parse_type(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_dot_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_next(parser);
    if (!nnc_parser_match(parser, TOK_IDENT)) {
        THROW(NNC_SYNTAX, "expected <TOK_IDENT> as member accessor.");
    }
    nnc_binary_expression* expr = nnc_binary_expr_new(BINARY_DOT);
    expr->lexpr = prefix;
    expr->rexpr = nnc_parse_primary_expr(parser);
    return nnc_expr_new(EXPR_BINARY, expr);
}

static nnc_expression* nnc_parse_call_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_OPAREN);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_CALL);
    expr->expr = prefix;
    while (!nnc_parser_match(parser, TOK_CPAREN) &&
           !nnc_parser_match(parser, TOK_EOF)) {
        buf_add(expr->exact.call.args, nnc_parse_expr_reduced(parser));
        if (nnc_parser_match(parser, TOK_CPAREN) ||
            nnc_parser_match(parser, TOK_EOF)) {
            continue;
        }
        nnc_parser_expect(parser, TOK_COMMA);
    }
    expr->exact.call.argc = buf_len(expr->exact.call.args);
    nnc_parser_expect(parser, TOK_CPAREN);
    return nnc_expr_new(EXPR_UNARY, expr);
}

static nnc_expression* nnc_parse_index_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_OBRACKET);
    nnc_binary_expression* expr = nnc_binary_expr_new(BINARY_IDX);
    expr->lexpr = prefix;
    expr->rexpr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_CBRACKET);
    return nnc_expr_new(EXPR_BINARY, expr);
}

static nnc_expression* nnc_parse_postfix_expr(nnc_parser* parser) {
    nnc_expression* postfix = NULL;
    nnc_expression* primary = nnc_parse_primary_expr(parser);
    if (nnc_parser_match(parser, TOK_AS)) {
        return nnc_parse_as_expr(parser, primary);
    }
    while (nnc_parser_match(parser, TOK_DOT)    ||
           nnc_parser_match(parser, TOK_OPAREN) ||
           nnc_parser_match(parser, TOK_OBRACKET)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        nnc_expression* prefix = postfix ? postfix : primary;
        switch (tok->kind) {
            case TOK_DOT:       postfix = nnc_parse_dot_expr(parser, prefix);    break;
            case TOK_OPAREN:    postfix = nnc_parse_call_expr(parser, prefix);   break;
            case TOK_OBRACKET:  postfix = nnc_parse_index_expr(parser, prefix);  break;
            default:
                nnc_abort_no_ctx("nnc_parse_postfix_expr: bug detected.\n");
        }
    }
    return postfix ? postfix : primary;
}

static nnc_expression* nnc_parse_unary_expr(nnc_parser* parser) {
    if (nnc_parser_match(parser, TOK_OPAREN)) {
        nnc_tok_kind lookup = nnc_parser_peek_lookup(parser);
        if (nnc_parser_match_type(lookup)) {
            return nnc_parse_cast_expr(parser);
        }
    }
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_PLUS:      return nnc_parse_plus_expr(parser);
        case TOK_MINUS:     return nnc_parse_minus_expr(parser);
        case TOK_TILDE:     return nnc_parse_bitwise_not_expr(parser);
        case TOK_ASTERISK:  return nnc_parse_dereference_expr(parser);
        case TOK_AMPERSAND: return nnc_parse_reference_expr(parser);
        case TOK_EXCMARK:   return nnc_parse_not_expr(parser);
        case TOK_SIZEOF:    return nnc_parse_sizeof_expr(parser);
        case TOK_LENGTHOF:  return nnc_parse_lengthof_expr(parser);
        default: 
            return nnc_parse_postfix_expr(parser);
    }
}

static nnc_expression* nnc_parse_arith_multiplication_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_unary_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_SLASH)   ||
           nnc_parser_match(parser, TOK_PERCENT) ||
           nnc_parser_match(parser, TOK_ASTERISK)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        nnc_binary_expression_kind kind = BINARY_DIV;
        switch (tok->kind) {
            case TOK_SLASH:    kind = BINARY_DIV; break;
            case TOK_PERCENT:  kind = BINARY_MOD; break;
            case TOK_ASTERISK: kind = BINARY_MUL; break;
            default:
                nnc_abort_no_ctx("nnc_parse_arith_multiplication_expr: bug detected.\n");
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_unary_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_arith_addition_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_arith_multiplication_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_PLUS) ||
           nnc_parser_match(parser, TOK_MINUS)) {
        nnc_binary_expression_kind kind = BINARY_ADD;
        if (nnc_parser_match(parser, TOK_MINUS)) {
            kind = BINARY_SUB;
        }
        // pub let pi = 3.141592f64;
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_arith_multiplication_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_arith_shift_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_arith_addition_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_LSHIFT) ||
           nnc_parser_match(parser, TOK_RSHIFT)) {
        nnc_binary_expression_kind kind = BINARY_SHL;
        if (nnc_parser_match(parser, TOK_RSHIFT)) {
            kind = BINARY_SHR;
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_arith_addition_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_relation_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_arith_shift_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_LT)  ||
           nnc_parser_match(parser, TOK_GT)  ||
           nnc_parser_match(parser, TOK_LTE) ||
           nnc_parser_match(parser, TOK_GTE)) {
        const nnc_tok* tok = nnc_parser_get(parser); 
        nnc_binary_expression_kind kind = BINARY_LT;
        switch (tok->kind) {
            case TOK_LT:  kind = BINARY_LT;  break;
            case TOK_GT:  kind = BINARY_GT;  break;
            case TOK_LTE: kind = BINARY_LTE; break;
            case TOK_GTE: kind = BINARY_GTE; break;
            default:
                nnc_abort_no_ctx("nnc_parse_relation_expr: bug detected.\n");
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_arith_shift_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_equality_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_relation_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_EQ)  ||
           nnc_parser_match(parser, TOK_NEQ)) {
        nnc_binary_expression_kind kind = BINARY_EQ;
        if (nnc_parser_match(parser, TOK_NEQ)) {
            kind = BINARY_NEQ;
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_relation_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_bitwise_and_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_equality_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_AMPERSAND)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_AND);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_equality_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_bitwise_xor_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_bitwise_and_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_CIRCUMFLEX)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_XOR);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_bitwise_and_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_bitwise_or_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_bitwise_xor_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_VLINE)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_OR);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_bitwise_xor_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_and_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_bitwise_or_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_AND)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_AND);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_bitwise_or_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_or_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_and_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_OR)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_OR);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_and_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_ternary_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_or_expr(parser);
    nnc_ternary_expression* expr = NULL;
    if (nnc_parser_match(parser, TOK_QUESTION)) {
        nnc_parser_next(parser);
        expr = nnc_ternary_expr_new();
        expr->lexpr = nnc_parse_expr(parser);
        nnc_parser_expect(parser, TOK_COLON);
        expr->rexpr = nnc_parse_expr(parser);
        expr->cexpr = nether_expr;   
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_TERNARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_assignment_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_ternary_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_ASSIGN)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_ASSIGN);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_ternary_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

static nnc_expression* nnc_parse_comma_expr(nnc_parser* parser) {
    nnc_expression* nether_expr = nnc_parse_assignment_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_COMMA)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_COMMA);
        temp->lexpr = nether_expr;
        temp->rexpr = nnc_parse_assignment_expr(parser);
        if (expr != NULL) {
            temp->lexpr = nnc_expr_new(EXPR_BINARY, expr);
        }
        expr = temp;
    }
    if (expr != NULL) {
        return nnc_expr_new(EXPR_BINARY, expr);
    }
    return nether_expr;
}

nnc_expression* nnc_parse_expr_reduced(nnc_parser* parser) {
    return nnc_parse_assignment_expr(parser);
}

nnc_expression* nnc_parse_expr(nnc_parser* parser) {
    return nnc_parse_comma_expr(parser);
}

nnc_statement* nnc_parse_expr_stmt(nnc_parser* parser);

nnc_cond_n_body* nnc_parse_cond_n_body(nnc_parser* parser) {
    nnc_cond_n_body* cond_n_body = new(nnc_cond_n_body);
    cond_n_body->cond = nnc_parse_parens(parser);
    cond_n_body->body = nnc_parse_stmt(parser);
    return cond_n_body;
}

nnc_statement* nnc_parse_if_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_IF);
    nnc_if_stmt* if_stmt = new(nnc_if_stmt);
    if_stmt->if_br = nnc_parse_cond_n_body(parser);
    while (nnc_parser_match(parser, TOK_ELIF)) {
        nnc_parser_next(parser);
        buf_add(if_stmt->elif_brs, nnc_parse_cond_n_body(parser));
    }
    if (nnc_parser_match(parser, TOK_ELSE)) {
        nnc_parser_next(parser);
        if_stmt->else_br = nnc_parse_stmt(parser);
    }
    return nnc_stmt_new(STMT_IF, if_stmt);
}

nnc_statement* nnc_parse_do_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_DO);
    nnc_do_while_stmt* do_stmt = new(nnc_do_while_stmt);
    do_stmt->body = nnc_parse_stmt(parser);
    nnc_parser_expect(parser, TOK_WHILE);
    do_stmt->cond = nnc_parse_parens(parser);
    return nnc_stmt_new(STMT_DO, do_stmt);
}

nnc_statement* nnc_parse_let_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_LET);
    nnc_let_statement* vardecl = new(nnc_let_statement);
    const nnc_tok* tok = nnc_parser_get(parser);
    vardecl->var = nnc_ident_new(tok->lexeme);
    nnc_parser_expect(parser, TOK_IDENT);
    nnc_parser_expect(parser, TOK_COLON);
    vardecl->type = nnc_parse_type(parser);
    if (nnc_parser_match(parser, TOK_ASSIGN)) {
        nnc_parser_expect(parser, TOK_ASSIGN);
        vardecl->init = nnc_parse_expr(parser);
    }
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_LET, vardecl);
}

nnc_statement* nnc_parse_for_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_FOR);
    nnc_for_stmt* for_stmt = new(nnc_for_stmt);
    nnc_parser_expect(parser, TOK_OPAREN);
    if (nnc_parser_match(parser, TOK_LET)) {
        for_stmt->init = nnc_parse_let_stmt(parser);
    }
    else {
        for_stmt->init = nnc_parse_expr_stmt(parser);
    }
    for_stmt->cond = nnc_parse_expr_stmt(parser);
    if (nnc_parser_match(parser, TOK_CPAREN)) {
        for_stmt->step = nnc_stmt_new(STMT_EMPTY, NULL);
    }
    else {
        nnc_expression_statement* exprstmt = new(nnc_expression_statement);
        exprstmt->expr = nnc_parse_expr(parser);
        for_stmt->step = nnc_stmt_new(STMT_EXPR, exprstmt);
    }
    nnc_parser_expect(parser, TOK_CPAREN);
    for_stmt->body = nnc_parse_stmt(parser);
    return nnc_stmt_new(STMT_FOR, for_stmt);
}

nnc_statement* nnc_parse_expr_stmt(nnc_parser* parser) {
    if (nnc_parser_match(parser, TOK_SEMICOLON)) {
        nnc_parser_next(parser);
        return nnc_stmt_new(STMT_EMPTY, NULL);
    }
    nnc_expression_statement* exprstmt = new(nnc_expression_statement);
    exprstmt->expr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_EXPR, exprstmt);
}

nnc_statement* nnc_parse_goto_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_GOTO);
    nnc_goto_statement* goto_stmt = new(nnc_goto_statement);
    goto_stmt->body = nnc_parse_expr_stmt(parser);
    nnc_expression_statement* body = goto_stmt->body->exact;
    if (goto_stmt->body->kind == STMT_EMPTY || 
        body->expr->kind != EXPR_IDENT) {
        THROW(NNC_SYNTAX, "expected label name.");
    }
    return nnc_stmt_new(STMT_GOTO, goto_stmt);
}

nnc_statement* nnc_parse_type_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_TYPE);
    nnc_type_statement* type_stmt = new(nnc_type_statement);
    type_stmt->type = nnc_parse_type(parser);
    nnc_parser_expect(parser, TOK_AS);
    type_stmt->as = nnc_parse_type(parser);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_TYPE, type_stmt);
}

nnc_statement* nnc_parse_while_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_WHILE);
    nnc_while_stmt* while_stmt = new(nnc_while_stmt);
    while_stmt->cond = nnc_parse_parens(parser);
    while_stmt->body = nnc_parse_stmt(parser);
    return nnc_stmt_new(STMT_WHILE, while_stmt);
}

nnc_statement* nnc_parse_break_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_BREAK);
    nnc_break_statement* break_stmt = new(nnc_break_statement);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_BREAK, break_stmt);
}

nnc_statement* nnc_parse_return_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_RETURN);
    nnc_return_statement* ret_stmt = new(nnc_return_statement);
    ret_stmt->body = nnc_parse_expr_stmt(parser);
    return nnc_stmt_new(STMT_RETURN, ret_stmt);
}

nnc_statement* nnc_parse_compound_stmt(nnc_parser* parser) {
    nnc_compound_statement* compound = new(nnc_compound_statement);
    nnc_parser_expect(parser, TOK_OBRACE);
    while (!nnc_parser_match(parser, TOK_CBRACE)) {
        buf_add(compound->stmts, nnc_parse_stmt(parser));
    }
    nnc_parser_expect(parser, TOK_CBRACE);
    return nnc_stmt_new(STMT_COMPOUND, compound);
}

nnc_statement* nnc_parse_continue_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_CONTINUE);
    nnc_continue_statement* continue_stmt = new(nnc_continue_statement);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_CONTINUE, continue_stmt);
}

nnc_statement* nnc_parse_stmt(nnc_parser* parser) {
    nnc_statement* stmt = NULL;
    // todo: replace `break` & `stmt` with `return`
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_IF:       stmt = nnc_parse_if_stmt(parser);       break;
        case TOK_DO:       stmt = nnc_parse_do_stmt(parser);       break;
        case TOK_LET:      stmt = nnc_parse_let_stmt(parser);      break;
        case TOK_FOR:      stmt = nnc_parse_for_stmt(parser);      break;
        case TOK_GOTO:     stmt = nnc_parse_goto_stmt(parser);     break;
        case TOK_TYPE:     stmt = nnc_parse_type_stmt(parser);     break;
        case TOK_WHILE:    stmt = nnc_parse_while_stmt(parser);    break;
        case TOK_BREAK:    stmt = nnc_parse_break_stmt(parser);    break;
        case TOK_RETURN:   stmt = nnc_parse_return_stmt(parser);   break;
        case TOK_OBRACE:   stmt = nnc_parse_compound_stmt(parser); break;
        case TOK_CONTINUE: stmt = nnc_parse_continue_stmt(parser); break;
        default:
            stmt = nnc_parse_expr_stmt(parser);
    }
    return stmt;
}

nnc_ast* nnc_parse(const char* file) {
    nnc_parser parser = { 0 };
    nnc_parser_init(&parser, file);
    nnc_parser_next(&parser);
    nnc_ast* ast = nnc_ast_new(file);
    ast->root = nnc_parse_stmt(&parser);
    nnc_parser_fini(&parser);
    return ast;
}