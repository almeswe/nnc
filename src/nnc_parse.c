#include "nnc_parse.h"

void nnc_parser_init(nnc_parser* out_parser, const char* file) {
    nnc_lex_init(&out_parser->lex, file);
    out_parser->st = anew(nnc_st);
    nnc_st_init(out_parser->st);
    out_parser->st->ctx = ST_CTX_GLOBAL;
    out_parser->lookup.is_first = true;
    nnc_parser_next(out_parser);
}

void nnc_parser_init_with_fp(nnc_parser* out_parser, FILE* fp) {
    nnc_lex_init_with_fp(&out_parser->lex, fp);
    out_parser->st = anew(nnc_st);
    nnc_st_init(out_parser->st);
    out_parser->lookup.is_first = true;
    nnc_parser_next(out_parser);
}

void nnc_parser_fini(nnc_parser* parser) {
    if (parser != NULL) {
        nnc_lex_fini(&parser->lex);
    }
}

nnc_tok* nnc_parser_get(nnc_parser* parser) {
    return &parser->current.tok;
}

nnc_ctx* nnc_parser_get_ctx(nnc_parser* parser) {
    // when trying to get current parser context
    // we need to shift current char by size of token
    // because when token is grabbed, it's last character captured,
    // but in practical use the position of first character is needed.
    //      <token>
    //            ^ (captured)
    //      <token>
    //      ^       (shifted, used in errors and warnings)
    nnc_ctx* cctx = &parser->current.ctx;
    nnc_tok* ctok = &parser->current.tok;
    parser->current.ctx_copy = *cctx;
    assert(parser->current.ctx_copy.hint_ch >= ctok->size);
    parser->current.ctx_copy.hint_ch -= ctok->size;
    return &parser->current.ctx_copy;
}

nnc_ctx* nnc_parser_get_ctx2(nnc_parser* parser) {
    nnc_ctx* cctx = &parser->current.ctx;
    const nnc_tok* ctok = nnc_parser_get(parser); 
    cctx->hint_sz = ctok->size;
    return cctx;
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
        parser->current.ctx = parser->lookup.ctx;
    }
    else {
        parser->lookup.is_first = false;
        parser->current.tok = parser->lex.ctok;
        parser->current.ctx = parser->lex.cctx;
        nnc_lex_next(&parser->lex);
    }
    parser->lookup.tok = parser->lex.ctok;
    parser->lookup.ctx = parser->lex.cctx;
    return parser->current.tok.kind;
}

nnc_tok_kind nnc_parser_expect(nnc_parser* parser, nnc_tok_kind kind) {
    nnc_tok_kind current = nnc_parser_peek(parser);
    if (current != kind) {
        THROW(NNC_SYNTAX, sformat("expected `%s`, but met `%s`.", 
            nnc_tok_str(kind), nnc_tok_str(current)), *nnc_parser_get_ctx(parser));
    }   
    nnc_parser_next(parser);
    return nnc_parser_peek(parser);
}

nnc_static nnc_bool nnc_parser_match_type(nnc_parser* parser, nnc_tok_kind kind) {
    switch (kind) {
        case TOK_VOID:
        case TOK_I8:  case TOK_I16:
        case TOK_I32: case TOK_I64:
        case TOK_U8:  case TOK_U16:
        case TOK_U32: case TOK_U64:
        case TOK_F32: case TOK_F64:
            return true;
        default: break;
    }
    if (kind == TOK_IDENT) {
        const char* lexeme = parser->lookup.tok.lexeme;
        return nnc_st_get_type(parser->st, lexeme) != NULL;
    }
    return false;
}

nnc_static void nnc_parser_enter_scope(nnc_parser* parser) {
    nnc_st* inner = anew(nnc_st);
    nnc_st_init(inner);
    inner->ctx = parser->st->ctx;
    inner->ref = parser->st->ref; 
    inner->root = parser->st;
    buf_add(parser->st->branches, inner);
    parser->st = inner;
}

nnc_static void nnc_parser_leave_scope(nnc_parser* parser) {
    if (parser->st->root == NULL) {
        nnc_abort_no_ctx("nnc_parser_leave_scope: root st is NULL.");
    }
    parser->st = parser->st->root;
}

nnc_static nnc_type* nnc_parse_type(nnc_parser* parser);
nnc_static nnc_type_expression* nnc_parse_type_expr(nnc_parser* parser);

nnc_static nnc_var_type* nnc_parse_var_type(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_var_type* var_type = anew(nnc_var_type);
    if (nnc_parser_match(parser, TOK_IDENT)) {
        var_type->var = nnc_ident_new(tok->lexeme);
        var_type->var->ctx = *nnc_parser_get_ctx(parser);
    }
    nnc_parser_expect(parser, TOK_IDENT);
    nnc_parser_expect(parser, TOK_COLON);
    var_type->texpr = nnc_parse_type_expr(parser);
    return var_type;
}

nnc_static nnc_fn_param* nnc_parse_fn_param(nnc_parser* parser) {
    nnc_fn_param* fn_param = nnc_parse_var_type(parser);
    fn_param->var->ictx = IDENT_FUNCTION_PARAM;
    return fn_param;
}

nnc_static nnc_struct_member* nnc_parse_struct_member(nnc_parser* parser) {
    return (nnc_struct_member*)nnc_parse_var_type(parser);
}

nnc_static nnc_enumerator* nnc_parse_enumerator(nnc_parser* parser, nnc_type* in_enum) {
    nnc_enumerator* enumerator = anew(nnc_enumerator);
    enumerator->in_enum = in_enum;
    if (nnc_parser_match(parser, TOK_IDENT)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        enumerator->var = nnc_ident_new(tok->lexeme);
        enumerator->var->ctx = *nnc_parser_get_ctx(parser);
        enumerator->var->ictx = IDENT_ENUMERATOR;
        enumerator->var->refs.enumerator = enumerator;
    }
    nnc_parser_expect(parser, TOK_IDENT);
    if (nnc_parser_match(parser, TOK_ASSIGN)) {
        nnc_parser_next(parser);
        enumerator->init = nnc_parse_expr_reduced(parser);
    }
    return enumerator;
}

nnc_static nnc_type* nnc_parse_fn_type(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_FN);
    nnc_type* type = nnc_fn_type_new();
    nnc_parser_expect(parser, TOK_OPAREN);
    while (!nnc_parser_match(parser, TOK_CPAREN) &&
           !nnc_parser_match(parser, TOK_EOF)) {
        buf_add(type->exact.fn.params, nnc_parse_type_expr(parser));
        if (nnc_parser_match(parser, TOK_CPAREN)) {
            break;
        }
        nnc_parser_expect(parser, TOK_COMMA);
    }
    type->exact.fn.paramc = buf_len(type->exact.fn.params);
    nnc_parser_expect(parser, TOK_CPAREN);
    nnc_parser_expect(parser, TOK_COLON);
    type->exact.fn.ret = nnc_parse_type_expr(parser);
    return type;
}

nnc_static nnc_type* nnc_parse_user_type(nnc_parser* parser) {
    nnc_tok* tok = nnc_parser_get(parser);
    nnc_ident* ident = nnc_ident_new(tok->lexeme);
    ident->ctx = *nnc_parser_get_ctx(parser);
    nnc_parser_expect(parser, TOK_IDENT);
    return nnc_type_new(ident->name);
}

nnc_static nnc_type* nnc_parse_enum_type(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_ENUM);
    nnc_parser_expect(parser, TOK_OBRACE);
    nnc_type* type = nnc_enum_type_new();
    while (!nnc_parser_match(parser, TOK_CBRACE) &&
           !nnc_parser_match(parser, TOK_EOF)) {
        nnc_enumerator* member = nnc_parse_enumerator(parser, type);
        buf_add(type->exact.enumeration.members, member);
        nnc_st_put(parser->st, member->var, ST_SYM_IDENT);
        if (!nnc_parser_match(parser, TOK_CBRACE)) {
            nnc_parser_expect(parser, TOK_COMMA);
        }
    }
    nnc_parser_expect(parser, TOK_CBRACE);
    type->exact.enumeration.memberc = buf_len(
        type->exact.enumeration.members);
    return type;
}

nnc_static nnc_type* nnc_parse_struct_or_union_type(nnc_parser* parser) {
    nnc_type* type = NULL;
    nnc_tok_kind kind = nnc_parser_peek(parser);
    switch (kind) {
        case TOK_UNION:  type = nnc_union_type_new();  break;
        case TOK_STRUCT: type = nnc_struct_type_new(); break;
        default: 
            THROW(NNC_SYNTAX, "expected `TOK_STRUCT` or `TOK_UNION`.", *nnc_parser_get_ctx(parser)); 
    }
    nnc_parser_next(parser);
    nnc_parser_expect(parser, TOK_OBRACE);
    while (!nnc_parser_match(parser, TOK_CBRACE) &&
           !nnc_parser_match(parser, TOK_EOF)) {
        buf_add(type->exact.struct_or_union.members, 
            nnc_parse_struct_member(parser));
        type->exact.struct_or_union.memberc++;
        nnc_parser_expect(parser, TOK_SEMICOLON);
    }
    nnc_parser_expect(parser, TOK_CBRACE);
    return type;
}

nnc_static nnc_type* nnc_parse_arr_declarator(nnc_parser* parser, nnc_type* type) {
    nnc_parser_expect(parser, TOK_OBRACKET);
    type = nnc_arr_type_new(type);
    type->exact.array.dim = nnc_parse_expr_reduced(parser);
    nnc_parser_expect(parser, TOK_CBRACKET);
    return type;
}

nnc_static nnc_type* nnc_parse_ptr_declarator(nnc_parser* parser, nnc_type* type) {
    nnc_parser_expect(parser, TOK_ASTERISK);
    type = nnc_ptr_type_new(type);
    if (type->base->kind == T_ARRAY) {
        THROW(NNC_SYNTAX, sformat("cannot declare type \'%s\'.", nnc_type_tostr(type)), *nnc_parser_get_ctx(parser));
    }
    return type;
}

nnc_static nnc_type* nnc_parse_type_declarators(nnc_parser* parser, nnc_type* type) {
    while (nnc_parser_match(parser, TOK_ASTERISK) ||
           nnc_parser_match(parser, TOK_OBRACKET)) {
        nnc_tok_kind kind = nnc_parser_peek(parser);
        switch (kind) {
            case TOK_OBRACKET: type = nnc_parse_arr_declarator(parser, type); break;    
            case TOK_ASTERISK: type = nnc_parse_ptr_declarator(parser, type); break;
            default: break;
        }
    }
    return type;
}

nnc_static nnc_type* nnc_parse_type(nnc_parser* parser) {
    nnc_type* type = NULL;
    const nnc_tok_kind kind = nnc_parser_peek(parser);
    switch (kind) {
        case TOK_I8:     type = &i8_type;   break;
        case TOK_U8:     type = &u8_type;   break;
        case TOK_I16:    type = &i16_type;  break;
        case TOK_U16:    type = &u16_type;  break;
        case TOK_I32:    type = &i32_type;  break;
        case TOK_U32:    type = &u32_type;  break;
        case TOK_I64:    type = &i64_type;  break;
        case TOK_U64:    type = &u64_type;  break;
        case TOK_F32:    type = &f32_type;  break;
        case TOK_F64:    type = &f64_type;  break;
        case TOK_VOID:   type = &void_type; break;
        case TOK_FN:     type = nnc_parse_fn_type(parser);   break;
        case TOK_IDENT:  type = nnc_parse_user_type(parser); break;
        case TOK_ENUM:   type = nnc_parse_enum_type(parser); break;
        case TOK_UNION:  type = nnc_parse_struct_or_union_type(parser); break;
        case TOK_STRUCT: type = nnc_parse_struct_or_union_type(parser); break;
        default: THROW(NNC_SYNTAX, "type expected.", *nnc_parser_get_ctx(parser));
    }
    // todo: fix this condition expression
    if (kind != TOK_IDENT &&
        nnc_parser_match_type(parser, kind)) {
        nnc_parser_next(parser);
    }
    return nnc_parse_type_declarators(parser, type);
}

nnc_static nnc_expression* nnc_parse_dbl(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_dbl_literal* exact = nnc_dbl_new(tok->lexeme, nnc_parser_get_ctx(parser));
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_DBL_LITERAL, exact);
}

nnc_static nnc_expression* nnc_parse_int(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_int_literal* exact = nnc_int_new(tok->lexeme, nnc_parser_get_ctx(parser));
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_INT_LITERAL, exact);
}

nnc_static nnc_expression* nnc_parse_chr(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_chr_literal* exact = nnc_chr_new(tok->lexeme, nnc_parser_get_ctx(parser));
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_CHR_LITERAL, exact);
}

nnc_static nnc_expression* nnc_parse_str(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_str_literal* exact = nnc_str_new(tok->lexeme, nnc_parser_get_ctx(parser));
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_STR_LITERAL, exact);
}

nnc_static nnc_nesting* nnc_parse_nesting(nnc_parser* parser) {
    nnc_nesting* root = NULL;
    nnc_nesting* nesting = NULL;
    while (nnc_parser_match(parser, TOK_IDENT)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        nnc_tok_kind peek = nnc_parser_peek_lookup(parser);
        if (peek != TOK_DCOLON) {
            break;
        }
        if (nesting == NULL) {
            nesting = anew(nnc_nesting);
            root = nesting;
        }
        else {
            nesting->next = anew(nnc_nesting);
            nesting = nesting->next;
        }
        nesting->ctx = *nnc_parser_get_ctx(parser);
        nesting->nest = nnc_ident_new(tok->lexeme);
        nesting->nest->ctx = nesting->ctx;
        nnc_parser_expect(parser, TOK_IDENT);
        nnc_parser_expect(parser, TOK_DCOLON);
    }
    return root;
}

nnc_static nnc_expression* nnc_parse_ident(nnc_parser* parser) {
    nnc_nesting* nesting = nnc_parse_nesting(parser);
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_ident* exact = nnc_ident_new(tok->lexeme);
    exact->nesting = nesting;
    exact->ctx = *nnc_parser_get_ctx(parser);
    nnc_parser_next(parser);
    return nnc_expr_new(EXPR_IDENT, exact);
}

nnc_static nnc_expression* nnc_parse_number(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    if (strchr(tok->lexeme, '.') != NULL) {
        return nnc_parse_dbl(parser);
    }
    return nnc_parse_int(parser);
}

nnc_static nnc_expression* nnc_parse_parens(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_OPAREN);
    nnc_expression* expr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    return expr;
}

nnc_static nnc_type_expression* nnc_parse_fn_ret_type_expr(nnc_parser* parser) {
    if (nnc_parser_match(parser, TOK_COLON)) {
        nnc_parser_expect(parser, TOK_COLON);
        return nnc_parse_type_expr(parser);
    }
    nnc_type_expression* type_expr = anew(nnc_type_expression);
    type_expr->ctx = *nnc_parser_get_ctx(parser);
    type_expr->type = &void_type;
    return type_expr;
}

nnc_static nnc_type_expression* nnc_parse_type_expr(nnc_parser* parser) {
    nnc_type_expression* type_expr = anew(nnc_type_expression);
    type_expr->nesting = nnc_parse_nesting(parser);
    type_expr->ctx = *nnc_parser_get_ctx(parser);
    type_expr->type = nnc_parse_type(parser);
    return type_expr;
}

nnc_static nnc_expression* nnc_parse_primary_expr(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_STR:    return nnc_parse_str(parser);
        case TOK_CHR:    return nnc_parse_chr(parser);
        case TOK_IDENT:  return nnc_parse_ident(parser);
        case TOK_NUMBER: return nnc_parse_number(parser);
        case TOK_OPAREN: return nnc_parse_parens(parser);
        default: THROW(NNC_SYNTAX, sformat("nnc_parse_primary_expr: %s.", 
            nnc_tok_str(tok->kind)), *nnc_parser_get_ctx(parser));
    }
    return NULL;
}

nnc_static nnc_expression* nnc_parse_unary_expr(nnc_parser* parser);

nnc_static nnc_expression* nnc_parse_cast_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_OPAREN);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_CAST);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->exact.cast.to = nnc_parse_type_expr(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_plus_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_PLUS);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_PLUS);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_minus_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_MINUS);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_MINUS);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_bitwise_not_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_TILDE);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_BITWISE_NOT);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

nnc_static nnc_expression* nnc_parse_deref_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_ASTERISK);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_DEREF);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

nnc_static nnc_expression* nnc_parse_ref_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_AMPERSAND);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_REF);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

nnc_static nnc_expression* nnc_parse_not_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_EXCMARK);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_NOT);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_unary_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr); 
}

nnc_static nnc_expression* nnc_parse_sizeof_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_SIZEOF);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_SIZEOF);
    expr->ctx = *nnc_parser_get_ctx(parser);
    nnc_parser_expect(parser, TOK_OPAREN);
    expr->exact.size.of = nnc_parse_type_expr(parser);
    nnc_parser_expect(parser, TOK_CPAREN);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_lengthof_expr(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_LENGTHOF);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_LENGTHOF);
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->expr = nnc_parse_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_as_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_AS);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_AS);
    expr->expr = prefix;
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->exact.cast.to = nnc_parse_type_expr(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_postfix_expr(nnc_parser* parser);

nnc_static nnc_expression* nnc_parse_dot_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_DOT);
    if (!nnc_parser_match(parser, TOK_IDENT)) {
        THROW(NNC_SYNTAX, "expected `TOK_IDENT` as member accessor.", *nnc_parser_get_ctx(parser));
    }
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_DOT);
    expr->expr = prefix;
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->exact.dot.member = nnc_parse_ident(parser);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_call_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_OPAREN);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_CALL);
    expr->expr = prefix;
    expr->ctx = *nnc_parser_get_ctx(parser);
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

nnc_static nnc_expression* nnc_parse_index_expr(nnc_parser* parser, nnc_expression* prefix) {
    nnc_parser_expect(parser, TOK_OBRACKET);
    nnc_unary_expression* expr = nnc_unary_expr_new(UNARY_POSTFIX_INDEX);
    expr->expr = prefix;
    expr->ctx = *nnc_parser_get_ctx(parser);
    expr->exact.index.expr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_CBRACKET);
    return nnc_expr_new(EXPR_UNARY, expr);
}

nnc_static nnc_expression* nnc_parse_postfix_expr(nnc_parser* parser) {
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
            default: nnc_abort_no_ctx("nnc_parse_postfix_expr: unknown kind.\n");
        }
    }
    return postfix ? postfix : primary;
}

nnc_static nnc_expression* nnc_parse_unary_expr(nnc_parser* parser) {
    if (nnc_parser_match(parser, TOK_OPAREN)) {
        nnc_tok_kind lookup = nnc_parser_peek_lookup(parser);
        if (nnc_parser_match_type(parser, lookup)) {
            return nnc_parse_cast_expr(parser);
        }
    }
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_PLUS:      return nnc_parse_plus_expr(parser);
        case TOK_MINUS:     return nnc_parse_minus_expr(parser);
        case TOK_TILDE:     return nnc_parse_bitwise_not_expr(parser);
        case TOK_SIZEOF:    return nnc_parse_sizeof_expr(parser);
        case TOK_EXCMARK:   return nnc_parse_not_expr(parser);
        case TOK_LENGTHOF:  return nnc_parse_lengthof_expr(parser);
        case TOK_ASTERISK:  return nnc_parse_deref_expr(parser);
        case TOK_AMPERSAND: return nnc_parse_ref_expr(parser);
        default:            return nnc_parse_postfix_expr(parser);
    }
}

nnc_static nnc_expression* nnc_parse_arith_multiplication_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
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
            default: nnc_abort_no_ctx("nnc_parse_arith_multiplication_expr: unknown kind.\n");
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_arith_addition_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_arith_multiplication_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_PLUS) ||
           nnc_parser_match(parser, TOK_MINUS)) {
        nnc_binary_expression_kind kind = BINARY_ADD;
        if (nnc_parser_match(parser, TOK_MINUS)) {
            kind = BINARY_SUB;
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_arith_shift_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
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
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_relation_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
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
            default: nnc_abort_no_ctx("nnc_parse_relation_expr: unknown kind.\n");
        }
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(kind);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_equality_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
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
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_bitwise_and_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_equality_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_AMPERSAND)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_AND);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_bitwise_xor_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_bitwise_and_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_CIRCUMFLEX)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_XOR);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_bitwise_or_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_bitwise_xor_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_VLINE)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_BW_OR);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_and_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_bitwise_or_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_AND)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_AND);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_or_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_and_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_OR)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_OR);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_ternary_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_or_expr(parser);
    nnc_ternary_expression* expr = NULL;
    if (nnc_parser_match(parser, TOK_QUESTION)) {
        nnc_parser_next(parser);
        expr = nnc_ternary_expr_new();
        expr->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_assignment_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_ternary_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_ASSIGN)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_ASSIGN);
        temp->ctx = ctx;
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

nnc_static nnc_expression* nnc_parse_comma_expr(nnc_parser* parser) {
    nnc_ctx ctx = *nnc_parser_get_ctx(parser);
    nnc_expression* nether_expr = nnc_parse_assignment_expr(parser);
    nnc_binary_expression* temp = NULL;
    nnc_binary_expression* expr = NULL;
    while (nnc_parser_match(parser, TOK_COMMA)) {
        nnc_parser_next(parser);
        temp = nnc_binary_expr_new(BINARY_COMMA);
        temp->ctx = ctx;
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

nnc_static nnc_statement* nnc_parse_expr_stmt(nnc_parser* parser);
nnc_static nnc_statement* nnc_parse_compound_stmt(nnc_parser* parser, nnc_st_ctx ctx);

nnc_static nnc_statement* nnc_parse_if_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_IF);
    nnc_if_statement* if_stmt = anew(nnc_if_statement);
    if_stmt->if_br = anew(nnc_if_branch);
    if_stmt->if_br->cond = nnc_parse_parens(parser);
    if_stmt->if_br->body = nnc_parse_compound_stmt(parser, ST_CTX_IF);
    while (nnc_parser_match(parser, TOK_ELIF)) {
        nnc_parser_next(parser);
        nnc_elif_branch* elif_br = anew(nnc_elif_branch);
        elif_br->cond = nnc_parse_parens(parser);
        elif_br->body = nnc_parse_compound_stmt(parser, ST_CTX_ELIF);
        buf_add(if_stmt->elif_brs, elif_br);
    }
    if (nnc_parser_match(parser, TOK_ELSE)) {
        nnc_parser_next(parser);
        if_stmt->else_br = nnc_parse_compound_stmt(parser, ST_CTX_ELSE);
    }
    return nnc_stmt_new(STMT_IF, if_stmt);
}

nnc_static nnc_statement* nnc_parse_do_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_DO);
    nnc_do_while_statement* do_stmt = anew(nnc_do_while_statement);
    do_stmt->body = nnc_parse_compound_stmt(parser, ST_CTX_LOOP);
    nnc_parser_expect(parser, TOK_WHILE);
    do_stmt->cond = nnc_parse_parens(parser);
    return nnc_stmt_new(STMT_DO, do_stmt);
}

nnc_static nnc_statement* nnc_parse_let_stmt_with_opt_st(nnc_parser* parser, nnc_bool put_in_st) {
    nnc_parser_expect(parser, TOK_LET);
    nnc_let_statement* let_stmt = anew(nnc_let_statement);
    const nnc_tok* tok = nnc_parser_get(parser);
    let_stmt->var = nnc_ident_new(tok->lexeme);
    let_stmt->var->ctx = *nnc_parser_get_ctx(parser);
    nnc_parser_expect(parser, TOK_IDENT);
    nnc_parser_expect(parser, TOK_COLON);
    let_stmt->texpr = nnc_parse_type_expr(parser);
    let_stmt->var->type = let_stmt->texpr->type; 
    if (nnc_parser_match(parser, TOK_ASSIGN)) {
        nnc_parser_expect(parser, TOK_ASSIGN);
        let_stmt->init = nnc_parse_expr(parser);
    }
    nnc_parser_expect(parser, TOK_SEMICOLON);
    if (put_in_st) {
        nnc_st_put(parser->st, let_stmt->var, ST_SYM_IDENT);
    }
    return nnc_stmt_new(STMT_LET, let_stmt);
}

nnc_static nnc_statement* nnc_parse_let_stmt(nnc_parser* parser) {
    return nnc_parse_let_stmt_with_opt_st(parser, true);
}

nnc_static nnc_statement* nnc_parse_for_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_FOR);
    nnc_for_statement* for_stmt = anew(nnc_for_statement);
    nnc_parser_expect(parser, TOK_OPAREN);
    if (nnc_parser_match(parser, TOK_LET)) {
        for_stmt->init = nnc_parse_let_stmt_with_opt_st(parser, false);
    }
    else {
        for_stmt->init = nnc_parse_expr_stmt(parser);
    }
    for_stmt->cond = nnc_parse_expr_stmt(parser);
    if (nnc_parser_match(parser, TOK_CPAREN)) {
        for_stmt->step = nnc_stmt_new(STMT_EMPTY, NULL);
    }
    else {
        nnc_expression_statement* expr_stmt = anew(nnc_expression_statement);
        expr_stmt->expr = nnc_parse_expr(parser);
        for_stmt->step = nnc_stmt_new(STMT_EXPR, expr_stmt);
    }
    nnc_parser_expect(parser, TOK_CPAREN);
    for_stmt->body = nnc_parse_compound_stmt(parser, ST_CTX_LOOP);
    if (for_stmt->init->kind == STMT_LET &&
        for_stmt->body->kind == STMT_COMPOUND) {
        nnc_st* inner = NNC_GET_SYMTABLE(for_stmt);
        nnc_st_put(inner, ((nnc_let_statement*)(for_stmt->init->exact))->var, ST_SYM_IDENT);
    }
    return nnc_stmt_new(STMT_FOR, for_stmt);
}

nnc_static nnc_statement* nnc_parse_expr_stmt(nnc_parser* parser) {
    if (nnc_parser_match(parser, TOK_SEMICOLON)) {
        nnc_parser_next(parser);
        return nnc_stmt_new(STMT_EMPTY, NULL);
    }
    nnc_expression_statement* exprstmt = anew(nnc_expression_statement);
    exprstmt->expr = nnc_parse_expr(parser);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_EXPR, exprstmt);
}

nnc_static nnc_statement* nnc_parse_goto_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_GOTO);
    nnc_goto_statement* goto_stmt = anew(nnc_goto_statement);
    goto_stmt->body = nnc_parse_expr_stmt(parser);
    nnc_expression_statement* body = goto_stmt->body->exact;
    if (goto_stmt->body->kind == STMT_EMPTY || 
        body->expr->kind != EXPR_IDENT) {
        nnc_parser_expect(parser, TOK_IDENT);
    }
    return nnc_stmt_new(STMT_GOTO, goto_stmt);
}

nnc_static nnc_statement* nnc_parse_type_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_TYPE);
    nnc_type_statement* type_stmt = anew(nnc_type_statement);
    type_stmt->texpr = nnc_parse_type_expr(parser);
    nnc_parser_expect(parser, TOK_AS);
    type_stmt->texpr_as = anew(nnc_type_expression);
    type_stmt->texpr_as->type = nnc_alias_type_new();
    type_stmt->texpr_as->type->base = type_stmt->texpr->type;
    if (nnc_parser_match(parser, TOK_IDENT)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        type_stmt->texpr_as->type->repr = nnc_sdup(tok->lexeme);
    }
    nnc_parser_expect(parser, TOK_IDENT);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    nnc_st_put(parser->st, type_stmt->texpr_as->type, ST_SYM_TYPE);
    return nnc_stmt_new(STMT_TYPE, type_stmt);
}

nnc_static nnc_statement* nnc_parse_while_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_WHILE);
    nnc_while_statement* while_stmt = anew(nnc_while_statement);
    while_stmt->cond = nnc_parse_parens(parser);
    while_stmt->body = nnc_parse_compound_stmt(parser, ST_CTX_LOOP);
    return nnc_stmt_new(STMT_WHILE, while_stmt);
}

nnc_static nnc_statement* nnc_parse_break_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_BREAK);
    nnc_break_statement* break_stmt = anew(nnc_break_statement);
    break_stmt->ctx = *nnc_parser_get_ctx(parser);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_BREAK, break_stmt);
}

nnc_static nnc_statement* nnc_parse_return_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_RETURN);
    nnc_return_statement* ret_stmt = anew(nnc_return_statement);
    ret_stmt->ctx = *nnc_parser_get_ctx(parser);
    ret_stmt->body = nnc_parse_expr_stmt(parser);
    return nnc_stmt_new(STMT_RETURN, ret_stmt);
}

nnc_static void nnc_parser_recover(nnc_parser* parser) {
    //nnc_lex_recover(&parser->lex);
    //nnc_parser_next(parser);
    while (!nnc_parser_match(parser, TOK_EOF)) {
        const nnc_tok* tok = nnc_parser_get(parser);
        if (tok->kind == TOK_SEMICOLON) {
            break;
        }
        nnc_parser_next(parser);
    }
}

nnc_static nnc_statement* nnc_parse_compound_stmt(nnc_parser* parser, nnc_st_ctx ctx) {
    nnc_parser_enter_scope(parser);
    nnc_compound_statement* compound_stmt = anew(nnc_compound_statement);
    compound_stmt->scope = parser->st;
    compound_stmt->scope->ctx = ctx;
    nnc_parser_expect(parser, TOK_OBRACE);
    while (!nnc_parser_match(parser, TOK_EOF) &&
           !nnc_parser_match(parser, TOK_CBRACE)) {
        buf_add(compound_stmt->stmts, nnc_parse_stmt(parser));
        /*
        nnc_statement* stmt = NULL;
        TRY {
            stmt = nnc_parse_stmt(parser);
            buf_add(compound_stmt->stmts, stmt);
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(CATCHED.where);
            nnc_parser_recover(parser);
        }
        */
    }
    nnc_parser_expect(parser, TOK_CBRACE);
    nnc_parser_leave_scope(parser);
    return nnc_stmt_new(STMT_COMPOUND, compound_stmt);
}

nnc_static nnc_statement* nnc_parse_continue_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_CONTINUE);
    nnc_continue_statement* continue_stmt = anew(nnc_continue_statement);
    continue_stmt->ctx = *nnc_parser_get_ctx(parser);
    nnc_parser_expect(parser, TOK_SEMICOLON);
    return nnc_stmt_new(STMT_CONTINUE, continue_stmt);
}

nnc_static nnc_statement* nnc_parse_fn_stmt(nnc_parser* parser) {
    //todo: specifiers like extern, static etc..
    nnc_parser_expect(parser, TOK_FN);
    const nnc_tok* tok = nnc_parser_get(parser);
    nnc_fn_statement* fn_stmt = anew(nnc_fn_statement);
    if (nnc_parser_match(parser, TOK_IDENT)) {
        fn_stmt->var = nnc_ident_new(tok->lexeme);
        fn_stmt->var->ctx = *nnc_parser_get_ctx(parser);
    }
    nnc_parser_expect(parser, TOK_IDENT);
    fn_stmt->var->ictx = IDENT_FUNCTION; 
    fn_stmt->var->type = nnc_fn_type_new();
    nnc_st_put(parser->st, fn_stmt->var, ST_SYM_IDENT);
    nnc_parser_expect(parser, TOK_OPAREN);
    while (!nnc_parser_match(parser, TOK_CPAREN) &&
           !nnc_parser_match(parser, TOK_EOF)) {
        nnc_fn_param* fn_param = nnc_parse_fn_param(parser);
        buf_add(fn_stmt->params, fn_param);
        buf_add(fn_stmt->var->type->exact.fn.params, fn_param->texpr);
        fn_stmt->var->type->exact.fn.paramc++;
        if (nnc_parser_match(parser, TOK_CPAREN)) {
            break;
        }
        nnc_parser_expect(parser, TOK_COMMA);
    }
    nnc_parser_expect(parser, TOK_CPAREN);
    fn_stmt->ret = nnc_parse_fn_ret_type_expr(parser);
    fn_stmt->var->type->exact.fn.ret = fn_stmt->ret;
    fn_stmt->body = nnc_parse_compound_stmt(parser, ST_CTX_FN);
    assert(fn_stmt->body->kind == STMT_COMPOUND);
    // put all function parameters inside inner scope of the function
    nnc_st* inner = NNC_GET_SYMTABLE(fn_stmt);
    inner->ref.fn = fn_stmt;
    for (nnc_u64 i = 0; i < buf_len(fn_stmt->params); i++) {
        TRY {
            nnc_st_put(inner, fn_stmt->params[i]->var, ST_SYM_IDENT);
            ETRY;
        }
        CATCHALL {
            NNC_SHOW_CATCHED(&CATCHED.where);
        }
    }
    return nnc_stmt_new(STMT_FN, fn_stmt);
}

nnc_static nnc_statement* nnc_parse_namespace_stmt(nnc_parser* parser) {
    nnc_parser_expect(parser, TOK_NAMESPACE);
    nnc_namespace_statement* namespace_stmt = anew(nnc_namespace_statement);
    const nnc_tok* tok = nnc_parser_get(parser);
    if (nnc_parser_match(parser, TOK_IDENT)) {
        namespace_stmt->var = nnc_ident_new(tok->lexeme);
        namespace_stmt->var->ctx = *nnc_parser_get_ctx(parser);
    }
    nnc_parser_expect(parser, TOK_IDENT);
    namespace_stmt->var->ictx = IDENT_NAMESPACE;
    namespace_stmt->var->type = nnc_namespace_type_new();
    namespace_stmt->var->type->repr = namespace_stmt->var->name;
    namespace_stmt->var->type->exact.name.space = namespace_stmt;
    namespace_stmt->body = nnc_parse_compound_stmt(parser, ST_CTX_NAMESPACE);
    nnc_st* inner = NNC_GET_SYMTABLE(namespace_stmt);
    inner->ref.np = namespace_stmt;
    nnc_st_put(parser->st, namespace_stmt->var, ST_SYM_IDENT);
    return nnc_stmt_new(STMT_NAMESPACE, namespace_stmt);
}

nnc_statement* nnc_parse_stmt(nnc_parser* parser) {
    const nnc_tok* tok = nnc_parser_get(parser);
    switch (tok->kind) {
        case TOK_IF:        return nnc_parse_if_stmt(parser);
        case TOK_FN:        return nnc_parse_fn_stmt(parser);
        case TOK_DO:        return nnc_parse_do_stmt(parser);
        case TOK_LET:       return nnc_parse_let_stmt(parser);
        case TOK_FOR:       return nnc_parse_for_stmt(parser);
        case TOK_GOTO:      return nnc_parse_goto_stmt(parser);
        case TOK_TYPE:      return nnc_parse_type_stmt(parser);
        case TOK_WHILE:     return nnc_parse_while_stmt(parser);
        case TOK_BREAK:     return nnc_parse_break_stmt(parser);
        case TOK_RETURN:    return nnc_parse_return_stmt(parser);
        case TOK_CONTINUE:  return nnc_parse_continue_stmt(parser);
        case TOK_NAMESPACE: return nnc_parse_namespace_stmt(parser);
        default:            return nnc_parse_expr_stmt(parser);
    }
}

nnc_ast* nnc_parse(const char* file) {
    nnc_parser parser = {0};
    nnc_parser_init(&parser, file);
    nnc_ast* ast = nnc_ast_new(file);
    while (nnc_parser_peek(&parser) != TOK_EOF) {
        buf_add(ast->root, nnc_parse_stmt(&parser));
    }
    ast->st = parser.st;
    nnc_parser_fini(&parser);
    return ast;
}