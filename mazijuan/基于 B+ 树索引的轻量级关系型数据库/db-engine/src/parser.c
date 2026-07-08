#include "parser.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    Lexer* lexer;
    Token current;
    int has_current;
} Parser;

static void* xmalloc(size_t n) {
    return calloc(1, n);
}

static char* dup_string(const char* s) {
    size_t n = strlen(s) + 1;
    char* out = (char*)malloc(n);
    if (out) memcpy(out, s, n);
    return out;
}

static void parser_init(Parser* p, Lexer* lx) {
    p->lexer = lx;
    p->has_current = 0;
    p->current.type = TK_EOF;
    p->current.text = NULL;
    p->current.int_value = 0;
}

static Token parser_peek(Parser* p) {
    if (!p->has_current) {
        p->current = lexer_next(p->lexer);
        p->has_current = 1;
    }
    return p->current;
}

static Token parser_next(Parser* p) {
    if (!p->has_current) {
        return lexer_next(p->lexer);
    }
    Token tok = p->current;
    p->has_current = 0;
    p->current.text = NULL;
    p->current.type = TK_EOF;
    p->current.int_value = 0;
    return tok;
}

static int match(Parser* p, TokenType type) {
    Token tok = parser_peek(p);
    if (tok.type != type) {
        return 0;
    }
    (void)parser_next(p);
    return 1;
}

static Token expect(Parser* p, TokenType type) {
    Token tok = parser_peek(p);
    if (tok.type != type) {
        Token empty = {TK_EOF, NULL, 0};
        return empty;
    }
    return parser_next(p);
}

static Expr* parse_expression(Parser* p) {
    Token tok = parser_peek(p);
    Expr* expr = NULL;
    if (tok.type == TK_INTEGER) {
        tok = parser_next(p);
        expr = (Expr*)xmalloc(sizeof(Expr));
        expr->type = EXPR_LITERAL_INT;
        expr->value.int_value = tok.int_value;
        token_free(&tok);
        return expr;
    }
    if (tok.type == TK_STRING) {
        tok = parser_next(p);
        expr = (Expr*)xmalloc(sizeof(Expr));
        expr->type = EXPR_LITERAL_STRING;
        expr->value.string_value = dup_string(tok.text ? tok.text : "");
        token_free(&tok);
        return expr;
    }
    if (tok.type == TK_IDENTIFIER) {
        tok = parser_next(p);
        expr = (Expr*)xmalloc(sizeof(Expr));
        expr->type = EXPR_IDENTIFIER;
        expr->value.identifier = dup_string(tok.text ? tok.text : "");
        token_free(&tok);
        return expr;
    }
    return NULL;
}

static Expr* parse_condition(Parser* p) {
    Expr* left = parse_expression(p);
    if (!left) return NULL;
    Token op = parser_peek(p);
    if (op.type == TK_EQ || op.type == TK_NE || op.type == TK_LT || op.type == TK_GT || op.type == TK_LE || op.type == TK_GE) {
        Token consumed = parser_next(p);
        Expr* right = parse_expression(p);
        Expr* expr = (Expr*)xmalloc(sizeof(Expr));
        expr->type = EXPR_BINARY;
        expr->value.binary.op = consumed.type;
        expr->value.binary.left = left;
        expr->value.binary.right = right;
        token_free(&consumed);
        return expr;
    }
    return left;
}

static char** parse_identifier_list(Parser* p, size_t* count) {
    char** items = NULL;
    size_t n = 0;
    if (match(p, TK_STAR)) {
        items = (char**)xmalloc(sizeof(char*));
        items[0] = dup_string("*");
        *count = 1;
        return items;
    }
    if (parser_peek(p).type != TK_IDENTIFIER) {
        *count = 0;
        return NULL;
    }
    while (parser_peek(p).type == TK_IDENTIFIER) {
        Token ident = parser_next(p);
        items = (char**)realloc(items, (n + 1) * sizeof(char*));
        items[n++] = dup_string(ident.text ? ident.text : "");
        token_free(&ident);
        if (!match(p, TK_COMMA)) break;
    }
    *count = n;
    return items;
}

static AstNode* parse_select(Parser* p) {
    AstNode* ast = (AstNode*)xmalloc(sizeof(AstNode));
    ast->type = AST_SELECT;
    ast->value.select.table_name = NULL;
    ast->value.select.select_all = 0;
    ast->value.select.columns = NULL;
    ast->value.select.column_count = 0;
    ast->value.select.where = NULL;

    if (!match(p, TK_SELECT)) {
        free(ast);
        return NULL;
    }
    ast->value.select.columns = parse_identifier_list(p, &ast->value.select.column_count);
    ast->value.select.select_all = (ast->value.select.column_count == 1 && ast->value.select.columns && strcmp(ast->value.select.columns[0], "*") == 0);
    if (!match(p, TK_FROM)) {
        ast_destroy(ast);
        return NULL;
    }
    Token table = expect(p, TK_IDENTIFIER);
    ast->value.select.table_name = dup_string(table.text ? table.text : "");
    token_free(&table);
    if (match(p, TK_WHERE)) {
        ast->value.select.where = parse_condition(p);
    }
    return ast;
}

static AstNode* parse_insert(Parser* p) {
    AstNode* ast = (AstNode*)xmalloc(sizeof(AstNode));
    ast->type = AST_INSERT;
    ast->value.insert.table_name = NULL;
    ast->value.insert.columns = NULL;
    ast->value.insert.column_count = 0;
    ast->value.insert.values = NULL;
    ast->value.insert.value_count = 0;

    if (!match(p, TK_INSERT) || !match(p, TK_INTO)) {
        free(ast);
        return NULL;
    }
    Token table = expect(p, TK_IDENTIFIER);
    ast->value.insert.table_name = dup_string(table.text ? table.text : "");
    token_free(&table);
    if (match(p, TK_LPAREN)) {
        ast->value.insert.columns = parse_identifier_list(p, &ast->value.insert.column_count);
        (void)match(p, TK_RPAREN);
    }
    if (!match(p, TK_VALUES)) {
        ast_destroy(ast);
        return NULL;
    }

    int has_parens = match(p, TK_LPAREN);
    size_t n = 0;
    while (parser_peek(p).type != TK_RPAREN && parser_peek(p).type != TK_EOF) {
        Expr* expr = parse_expression(p);
        if (!expr) break;
        ast->value.insert.values = (Expr**)realloc(ast->value.insert.values, (n + 1) * sizeof(Expr*));
        ast->value.insert.values[n++] = expr;
        if (!match(p, TK_COMMA)) break;
    }
    ast->value.insert.value_count = n;
    if (has_parens) {
        (void)match(p, TK_RPAREN);
    }
    return ast;
}

static AstNode* parse_delete(Parser* p) {
    AstNode* ast = (AstNode*)xmalloc(sizeof(AstNode));
    ast->type = AST_DELETE;
    ast->value.delete.table_name = NULL;
    ast->value.delete.where = NULL;
    if (!match(p, TK_DELETE) || !match(p, TK_FROM)) {
        free(ast);
        return NULL;
    }
    Token table = expect(p, TK_IDENTIFIER);
    ast->value.delete.table_name = dup_string(table.text ? table.text : "");
    token_free(&table);
    if (match(p, TK_WHERE)) {
        ast->value.delete.where = parse_condition(p);
    }
    return ast;
}

static AstNode* parse_update(Parser* p) {
    AstNode* ast = (AstNode*)xmalloc(sizeof(AstNode));
    ast->type = AST_UPDATE;
    ast->value.update.table_name = NULL;
    ast->value.update.column_name = NULL;
    ast->value.update.value = NULL;
    ast->value.update.where = NULL;
    if (!match(p, TK_UPDATE)) {
        free(ast);
        return NULL;
    }
    Token table = expect(p, TK_IDENTIFIER);
    ast->value.update.table_name = dup_string(table.text ? table.text : "");
    token_free(&table);
    if (!match(p, TK_SET)) {
        ast_destroy(ast);
        return NULL;
    }
    Token column = expect(p, TK_IDENTIFIER);
    ast->value.update.column_name = dup_string(column.text ? column.text : "");
    token_free(&column);
    if (!match(p, TK_EQ)) {
        ast_destroy(ast);
        return NULL;
    }
    ast->value.update.value = parse_expression(p);
    if (match(p, TK_WHERE)) {
        ast->value.update.where = parse_condition(p);
    }
    return ast;
}

AstNode* parse_sql(const char* sql) {
    Lexer* lx = lexer_create(sql);
    if (!lx) return NULL;

    Parser p;
    parser_init(&p, lx);

    Token first = parser_peek(&p);
    AstNode* ast = NULL;
    if (first.type == TK_SELECT) {
        ast = parse_select(&p);
    } else if (first.type == TK_INSERT) {
        ast = parse_insert(&p);
    } else if (first.type == TK_DELETE) {
        ast = parse_delete(&p);
    } else if (first.type == TK_UPDATE) {
        ast = parse_update(&p);
    }
    token_free(&first);
    lexer_destroy(lx);
    return ast;
}

void expr_destroy(Expr* expr) {
    if (!expr) return;
    if (expr->type == EXPR_LITERAL_STRING && expr->value.string_value) free(expr->value.string_value);
    if (expr->type == EXPR_IDENTIFIER && expr->value.identifier) free(expr->value.identifier);
    if (expr->type == EXPR_BINARY) {
        expr_destroy(expr->value.binary.left);
        expr_destroy(expr->value.binary.right);
    }
    free(expr);
}

void ast_destroy(AstNode* ast) {
    if (!ast) return;
    if (ast->type == AST_SELECT) {
        free(ast->value.select.table_name);
        if (ast->value.select.columns) {
            for (size_t i = 0; i < ast->value.select.column_count; ++i) free(ast->value.select.columns[i]);
            free(ast->value.select.columns);
        }
        expr_destroy(ast->value.select.where);
    } else if (ast->type == AST_INSERT) {
        free(ast->value.insert.table_name);
        if (ast->value.insert.columns) {
            for (size_t i = 0; i < ast->value.insert.column_count; ++i) free(ast->value.insert.columns[i]);
            free(ast->value.insert.columns);
        }
        if (ast->value.insert.values) {
            for (size_t i = 0; i < ast->value.insert.value_count; ++i) expr_destroy(ast->value.insert.values[i]);
            free(ast->value.insert.values);
        }
    } else if (ast->type == AST_DELETE) {
        free(ast->value.delete.table_name);
        expr_destroy(ast->value.delete.where);
    } else if (ast->type == AST_UPDATE) {
        free(ast->value.update.table_name);
        free(ast->value.update.column_name);
        expr_destroy(ast->value.update.value);
        expr_destroy(ast->value.update.where);
    }
    free(ast);
}
