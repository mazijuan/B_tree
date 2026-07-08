#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_eat(Parser* parser, TokenType type);
static Expr* parser_parse_expr(Parser* parser);
static Expr* parser_parse_comparison(Parser* parser);
static Expr* parser_parse_primary(Parser* parser);
static char** parser_parse_column_list(Parser* parser, int* count);
static Expr** parser_parse_value_list(Parser* parser, int* count);
static char* safe_strdup(const char* s);

// 安全字符串拷贝，分配失败返回NULL
static char* safe_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* buf = (char*)malloc(len + 1);
    if (!buf) {
        fprintf(stderr, "malloc failed: strdup\n");
        return NULL;
    }
    memcpy(buf, s, len);
    buf[len] = '\0';
    return buf;
}

Parser* parser_init(Lexer* lexer) {
    if (!lexer) return NULL;
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) {
        fprintf(stderr, "malloc failed: Parser\n");
        return NULL;
    }
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    return parser;
}

void parser_destroy(Parser* parser) {
    if (!parser) return;
    if (parser->current_token) {
        token_destroy(parser->current_token);
        parser->current_token = NULL;
    }
    free(parser);
}

static void parser_eat(Parser* parser, TokenType type) {
    if (!parser || !parser->current_token) return;
    if (parser->current_token->type == type) {
        token_destroy(parser->current_token);
        parser->current_token = lexer_next_token(parser->lexer);
        return;
    }
    // 匹配失败简单报错，不中断解析
    fprintf(stderr, "Syntax error: expected token type %d, got %d\n",
            type, parser->current_token->type);
}

static char** parser_parse_column_list(Parser* parser, int* count) {
    *count = 0;
    char** columns = NULL;

    parser_eat(parser, TK_LPAREN);

    while (parser->current_token && parser->current_token->type == TK_IDENTIFIER) {
        char** temp = (char**)realloc(columns, (*count + 1) * sizeof(char*));
        if (!temp) {
            fprintf(stderr, "realloc failed: column list\n");
            break;
        }
        columns = temp;
        columns[*count] = safe_strdup(parser->current_token->value);
        (*count)++;
        parser_eat(parser, TK_IDENTIFIER);

        if (parser->current_token && parser->current_token->type == TK_COMMA) {
            parser_eat(parser, TK_COMMA);
        } else {
            break;
        }
    }

    parser_eat(parser, TK_RPAREN);
    return columns;
}

static Expr** parser_parse_value_list(Parser* parser, int* count) {
    *count = 0;
    Expr** values = NULL;

    parser_eat(parser, TK_LPAREN);

    while (parser->current_token && parser->current_token->type != TK_RPAREN) {
        Expr** temp = (Expr**)realloc(values, (*count + 1) * sizeof(Expr*));
        if (!temp) {
            fprintf(stderr, "realloc failed: value list\n");
            break;
        }
        values = temp;
        values[*count] = parser_parse_primary(parser);
        (*count)++;

        if (parser->current_token && parser->current_token->type == TK_COMMA) {
            parser_eat(parser, TK_COMMA);
        } else {
            break;
        }
    }

    parser_eat(parser, TK_RPAREN);
    return values;
}

static Expr* parser_parse_primary(Parser* parser) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    if (!expr) {
        fprintf(stderr, "malloc failed: Expr primary\n");
        return NULL;
    }
    memset(expr, 0, sizeof(Expr)); // 初始化联合体，避免脏数据

    switch (parser->current_token->type) {
        case TK_IDENTIFIER:
            expr->op = OP_IDENTIFIER;
            expr->value.identifier = safe_strdup(parser->current_token->value);
            parser_eat(parser, TK_IDENTIFIER);
            break;
        case TK_INTEGER:
            expr->op = OP_INTEGER;
            expr->value.integer = atoi(parser->current_token->value);
            parser_eat(parser, TK_INTEGER);
            break;
        case TK_STRING:
            expr->op = OP_STRING;
            expr->value.string = safe_strdup(parser->current_token->value);
            parser_eat(parser, TK_STRING);
            break;
        case TK_LPAREN:
            parser_eat(parser, TK_LPAREN);
            // 释放空壳expr，替换为括号内表达式，修复内存泄漏
            free(expr);
            expr = parser_parse_expr(parser);
            parser_eat(parser, TK_RPAREN);
            break;
        default:
            expr->op = OP_IDENTIFIER;
            expr->value.identifier = safe_strdup("");
            break;
    }

    return expr;
}

static Expr* parser_parse_comparison(Parser* parser) {
    Expr* left = parser_parse_primary(parser);
    if (!left) return NULL;

    ExprOp op;
    switch (parser->current_token->type) {
        case TK_EQ:  op = OP_EQ;  parser_eat(parser, TK_EQ); break;
        case TK_NE:  op = OP_NE;  parser_eat(parser, TK_NE); break;
        case TK_LT:  op = OP_LT;  parser_eat(parser, TK_LT); break;
        case TK_GT:  op = OP_GT;  parser_eat(parser, TK_GT); break;
        case TK_LE:  op = OP_LE;  parser_eat(parser, TK_LE); break;
        case TK_GE:  op = OP_GE;  parser_eat(parser, TK_GE); break;
        default:
            return left;
    }

    Expr* right = parser_parse_primary(parser);
    if (!right) return left;

    Expr* result = (Expr*)malloc(sizeof(Expr));
    if (!result) {
        fprintf(stderr, "malloc failed: comparison expr\n");
        return left;
    }
    result->op = op;
    result->value.binary.left = left;
    result->value.binary.right = right;

    return result;
}

static Expr* parser_parse_expr(Parser* parser) {
    Expr* left = parser_parse_comparison(parser);
    if (!left) return NULL;

    while (parser->current_token) {
        TokenType t = parser->current_token->type;
        if (t != TK_AND && t != TK_OR) break;

        ExprOp op = (t == TK_AND) ? OP_AND : OP_OR;
        parser_eat(parser, t);

        Expr* right = parser_parse_comparison(parser);
        if (!right) break;

        Expr* result = (Expr*)malloc(sizeof(Expr));
        if (!result) {
            fprintf(stderr, "malloc failed: logic expr\n");
            break;
        }
        result->op = op;
        result->value.binary.left = left;
        result->value.binary.right = right;
        left = result;
    }

    return left;
}

static ASTNode* parser_parse_select(Parser* parser) {
    parser_eat(parser, TK_SELECT);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "malloc failed: SELECT ASTNode\n");
        return NULL;
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_SELECT;
    node->value.select.columns = NULL;
    node->value.select.column_count = 0;
    node->value.select.where = NULL;

    if (parser->current_token->type == TK_ASTERISK) {
        node->value.select.columns = (char**)malloc(sizeof(char*));
        if (node->value.select.columns) {
            node->value.select.columns[0] = safe_strdup("*");
            node->value.select.column_count = 1;
        }
        parser_eat(parser, TK_ASTERISK);
    } else {
        while (parser->current_token && parser->current_token->type == TK_IDENTIFIER) {
            char** temp = (char**)realloc(
                node->value.select.columns,
                (node->value.select.column_count + 1) * sizeof(char*)
            );
            if (!temp) break;
            node->value.select.columns = temp;
            node->value.select.columns[node->value.select.column_count] =
                safe_strdup(parser->current_token->value);
            node->value.select.column_count++;
            parser_eat(parser, TK_IDENTIFIER);

            if (parser->current_token && parser->current_token->type == TK_COMMA) {
                parser_eat(parser, TK_COMMA);
            } else {
                break;
            }
        }
    }

    parser_eat(parser, TK_FROM);
    if (parser->current_token) {
        node->value.select.table = safe_strdup(parser->current_token->value);
        parser_eat(parser, TK_IDENTIFIER);
    }

    if (parser->current_token && parser->current_token->type == TK_WHERE) {
        parser_eat(parser, TK_WHERE);
        node->value.select.where = parser_parse_expr(parser);
    }

    return node;
}

static ASTNode* parser_parse_insert(Parser* parser) {
    parser_eat(parser, TK_INSERT);
    parser_eat(parser, TK_INTO);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "malloc failed: INSERT ASTNode\n");
        return NULL;
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_INSERT;

    if (parser->current_token) {
        node->value.insert.table = safe_strdup(parser->current_token->value);
        parser_eat(parser, TK_IDENTIFIER);
    }

    int col_count = 0;
    node->value.insert.columns = parser_parse_column_list(parser, &col_count);
    node->value.insert.column_count = col_count;

    parser_eat(parser, TK_VALUES);

    int val_count = 0;
    node->value.insert.values = parser_parse_value_list(parser, &val_count);
    node->value.insert.value_count = val_count;

    return node;
}

static ASTNode* parser_parse_delete(Parser* parser) {
    parser_eat(parser, TK_DELETE);
    parser_eat(parser, TK_FROM);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "malloc failed: DELETE ASTNode\n");
        return NULL;
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_DELETE;

    if (parser->current_token) {
        node->value.delete.table = safe_strdup(parser->current_token->value);
        parser_eat(parser, TK_IDENTIFIER);
    }

    if (parser->current_token && parser->current_token->type == TK_WHERE) {
        parser_eat(parser, TK_WHERE);
        node->value.delete.where = parser_parse_expr(parser);
    }

    return node;
}

static ASTNode* parser_parse_update(Parser* parser) {
    parser_eat(parser, TK_UPDATE);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "malloc failed: UPDATE ASTNode\n");
        return NULL;
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_UPDATE;
    node->value.update.columns = NULL;
    node->value.update.values = NULL;
    node->value.update.set_count = 0;
    node->value.update.where = NULL;

    if (parser->current_token) {
        node->value.update.table = safe_strdup(parser->current_token->value);
        parser_eat(parser, TK_IDENTIFIER);
    }

    parser_eat(parser, TK_SET);

    while (parser->current_token && parser->current_token->type == TK_IDENTIFIER) {
        char* column = safe_strdup(parser->current_token->value);
        parser_eat(parser, TK_IDENTIFIER);
        parser_eat(parser, TK_EQ);

        Expr* value = parser_parse_primary(parser);

        char** col_temp = (char**)realloc(
            node->value.update.columns,
            (node->value.update.set_count + 1) * sizeof(char*)
        );
        Expr** val_temp = (Expr**)realloc(
            node->value.update.values,
            (node->value.update.set_count + 1) * sizeof(Expr*)
        );
        if (!col_temp || !val_temp) break;

        node->value.update.columns = col_temp;
        node->value.update.values = val_temp;
        node->value.update.columns[node->value.update.set_count] = column;
        node->value.update.values[node->value.update.set_count] = value;
        node->value.update.set_count++;

        if (parser->current_token && parser->current_token->type == TK_COMMA) {
            parser_eat(parser, TK_COMMA);
        } else {
            break;
        }
    }

    if (parser->current_token && parser->current_token->type == TK_WHERE) {
        parser_eat(parser, TK_WHERE);
        node->value.update.where = parser_parse_expr(parser);
    }

    return node;
}

ASTNode* parser_parse(Parser* parser) {
    if (!parser || !parser->current_token) return NULL;
    switch (parser->current_token->type) {
        case TK_SELECT:  return parser_parse_select(parser);
        case TK_INSERT:  return parser_parse_insert(parser);
        case TK_DELETE:  return parser_parse_delete(parser);
        case TK_UPDATE:  return parser_parse_update(parser);
        default:
            fprintf(stderr, "Unsupported SQL keyword token\n");
            return NULL;
    }
}

static void expr_destroy(Expr* expr) {
    if (!expr) return;

    switch (expr->op) {
        case OP_AND:
        case OP_OR:
        case OP_EQ:
        case OP_NE:
        case OP_LT:
        case OP_GT:
        case OP_LE:
        case OP_GE:
            expr_destroy(expr->value.binary.left);
            expr_destroy(expr->value.binary.right);
            break;
        case OP_IDENTIFIER:
            free(expr->value.identifier);
            break;
        case OP_STRING:
            free(expr->value.string);
            break;
        case OP_INTEGER:
            break;
        default:
            break;
    }
    free(expr);
}

void ast_destroy(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case NODE_SELECT: {
            SelectStmt* s = &node->value.select;
            if (s->columns) {
                for (int i = 0; i < s->column_count; i++) free(s->columns[i]);
                free(s->columns);
            }
            free(s->table);
            expr_destroy(s->where);
            break;
        }
        case NODE_INSERT: {
            InsertStmt* ins = &node->value.insert;
            free(ins->table);
            if (ins->columns) {
                for (int i = 0; i < ins->column_count; i++) free(ins->columns[i]);
                free(ins->columns);
            }
            if (ins->values) {
                for (int i = 0; i < ins->value_count; i++) expr_destroy(ins->values[i]);
                free(ins->values);
            }
            break;
        }
        case NODE_DELETE: {
            DeleteStmt* d = &node->value.delete;
            free(d->table);
            expr_destroy(d->where);
            break;
        }
        case NODE_UPDATE: {
            UpdateStmt* u = &node->value.update;
            free(u->table);
            if (u->columns) {
                for (int i = 0; i < u->set_count; i++) free(u->columns[i]);
                free(u->columns);
            }
            if (u->values) {
                for (int i = 0; i < u->set_count; i++) expr_destroy(u->values[i]);
                free(u->values);
            }
            expr_destroy(u->where);
            break;
        }
        case NODE_IDENTIFIER:
            free(node->value.identifier);
            break;
        case NODE_STRING:
            free(node->value.string);
            break;
        default:
            break;
    }
    free(node);
}
