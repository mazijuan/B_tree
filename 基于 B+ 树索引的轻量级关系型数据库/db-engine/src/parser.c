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

Parser* parser_init(Lexer* lexer) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    return parser;
}

void parser_destroy(Parser* parser) {
    if (parser) {
        if (parser->current_token) {
            token_destroy(parser->current_token);
        }
        free(parser);
    }
}

static void parser_eat(Parser* parser, TokenType type) {
    if (parser->current_token->type == type) {
        token_destroy(parser->current_token);
        parser->current_token = lexer_next_token(parser->lexer);
    }
}

static char** parser_parse_column_list(Parser* parser, int* count) {
    *count = 0;
    char** columns = NULL;

    parser_eat(parser, TK_LPAREN);

    while (parser->current_token->type == TK_IDENTIFIER) {
        columns = (char**)realloc(columns, (*count + 1) * sizeof(char*));
        columns[*count] = strdup(parser->current_token->value);
        (*count)++;
        parser_eat(parser, TK_IDENTIFIER);

        if (parser->current_token->type == TK_COMMA) {
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

    while (parser->current_token->type != TK_RPAREN) {
        values = (Expr**)realloc(values, (*count + 1) * sizeof(Expr*));
        values[*count] = parser_parse_primary(parser);
        (*count)++;

        if (parser->current_token->type == TK_COMMA) {
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

    switch (parser->current_token->type) {
        case TK_IDENTIFIER:
            expr->op = OP_IDENTIFIER;
            expr->value.identifier = strdup(parser->current_token->value);
            parser_eat(parser, TK_IDENTIFIER);
            break;
        case TK_INTEGER:
            expr->op = OP_INTEGER;
            expr->value.integer = atoi(parser->current_token->value);
            parser_eat(parser, TK_INTEGER);
            break;
        case TK_STRING:
            expr->op = OP_STRING;
            expr->value.string = strdup(parser->current_token->value);
            parser_eat(parser, TK_STRING);
            break;
        case TK_LPAREN:
            parser_eat(parser, TK_LPAREN);
            expr = parser_parse_expr(parser);
            parser_eat(parser, TK_RPAREN);
            break;
        default:
            expr->op = OP_IDENTIFIER;
            expr->value.identifier = strdup("");
            break;
    }

    return expr;
}

static Expr* parser_parse_comparison(Parser* parser) {
    Expr* left = parser_parse_primary(parser);

    ExprOp op = OP_IDENTIFIER;
    switch (parser->current_token->type) {
        case TK_EQ:
            op = OP_EQ;
            parser_eat(parser, TK_EQ);
            break;
        case TK_NE:
            op = OP_NE;
            parser_eat(parser, TK_NE);
            break;
        case TK_LT:
            op = OP_LT;
            parser_eat(parser, TK_LT);
            break;
        case TK_GT:
            op = OP_GT;
            parser_eat(parser, TK_GT);
            break;
        case TK_LE:
            op = OP_LE;
            parser_eat(parser, TK_LE);
            break;
        case TK_GE:
            op = OP_GE;
            parser_eat(parser, TK_GE);
            break;
        default:
            return left;
    }

    Expr* right = parser_parse_primary(parser);

    Expr* result = (Expr*)malloc(sizeof(Expr));
    result->op = op;
    result->value.binary.left = left;
    result->value.binary.right = right;

    return result;
}

static Expr* parser_parse_expr(Parser* parser) {
    Expr* left = parser_parse_comparison(parser);

    while (parser->current_token->type == TK_AND || parser->current_token->type == TK_OR) {
        ExprOp op = parser->current_token->type == TK_AND ? OP_AND : OP_OR;
        parser_eat(parser, parser->current_token->type);

        Expr* right = parser_parse_comparison(parser);

        Expr* result = (Expr*)malloc(sizeof(Expr));
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
    node->type = NODE_SELECT;
    node->value.select.columns = NULL;
    node->value.select.column_count = 0;

    if (parser->current_token->type == TK_ASTERISK) {
        node->value.select.columns = (char**)malloc(sizeof(char*));
        node->value.select.columns[0] = strdup("*");
        node->value.select.column_count = 1;
        parser_eat(parser, TK_ASTERISK);
    } else {
        while (parser->current_token->type == TK_IDENTIFIER) {
            node->value.select.columns = (char**)realloc(
                node->value.select.columns,
                (node->value.select.column_count + 1) * sizeof(char*)
            );
            node->value.select.columns[node->value.select.column_count] = 
                strdup(parser->current_token->value);
            node->value.select.column_count++;
            parser_eat(parser, TK_IDENTIFIER);

            if (parser->current_token->type == TK_COMMA) {
                parser_eat(parser, TK_COMMA);
            } else {
                break;
            }
        }
    }

    parser_eat(parser, TK_FROM);
    node->value.select.table = strdup(parser->current_token->value);
    parser_eat(parser, TK_IDENTIFIER);

    if (parser->current_token->type == TK_WHERE) {
        parser_eat(parser, TK_WHERE);
        node->value.select.where = parser_parse_expr(parser);
    } else {
        node->value.select.where = NULL;
    }

    return node;
}

static ASTNode* parser_parse_insert(Parser* parser) {
    parser_eat(parser, TK_INSERT);
    parser_eat(parser, TK_INTO);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = NODE_INSERT;
    node->value.insert.table = strdup(parser->current_token->value);
    parser_eat(parser, TK_IDENTIFIER);

    int col_count;
    node->value.insert.columns = parser_parse_column_list(parser, &col_count);
    node->value.insert.column_count = col_count;

    parser_eat(parser, TK_VALUES);

    int val_count;
    node->value.insert.values = parser_parse_value_list(parser, &val_count);
    node->value.insert.value_count = val_count;

    return node;
}

static ASTNode* parser_parse_delete(Parser* parser) {
    parser_eat(parser, TK_DELETE);
    parser_eat(parser, TK_FROM);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = NODE_DELETE;
    node->value.delete.table = strdup(parser->current_token->value);
    parser_eat(parser, TK_IDENTIFIER);

    if (parser->current_token->type == TK_WHERE) {
        parser_eat(parser, TK_WHERE);
        node->value.delete.where = parser_parse_expr(parser);
    } else {
        node->value.delete.where = NULL;
    }

    return node;
}

static ASTNode* parser_parse_update(Parser* parser) {
    parser_eat(parser, TK_UPDATE);

    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = NODE_UPDATE;
    node->value.update.table = strdup(parser->current_token->value);
    parser_eat(parser, TK_IDENTIFIER);

    parser_eat(parser, TK_SET);

    node->value.update.columns = NULL;
    node->value.update.values = NULL;
    node->value.update.set_count = 0;

    while (parser->current_token->type == TK_IDENTIFIER) {
        char* column = strdup(parser->current_token->value);
        parser_eat(parser, TK_IDENTIFIER);
        parser_eat(parser, TK_EQ);

        Expr* value = parser_parse_primary(parser);

        node->value.update.columns = (char**)realloc(
            node->value.update.columns,
            (node->value.update.set_count + 1) * sizeof(char*)
        );
        node->value.update.values = (Expr**)realloc(
            node->value.update.values,
            (node->value.update.set_count + 1) * sizeof(Expr*)
        );
        node->value.update.columns[node->value.update.set_count] = column;
        node->value.update.values[node->value.update.set_count] = value;
        node->value.update.set_count++;

        if (parser->current_token->type == TK_COMMA) {
            parser_eat(parser, TK_COMMA);
        } else {
            break;
        }
    }

    if (parser->current_token->type == TK_WHERE) {
        parser_eat(parser, TK_WHERE);
        node->value.update.where = parser_parse_expr(parser);
    } else {
        node->value.update.where = NULL;
    }

    return node;
}

ASTNode* parser_parse(Parser* parser) {
    switch (parser->current_token->type) {
        case TK_SELECT:
            return parser_parse_select(parser);
        case TK_INSERT:
            return parser_parse_insert(parser);
        case TK_DELETE:
            return parser_parse_delete(parser);
        case TK_UPDATE:
            return parser_parse_update(parser);
        default:
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
            if (expr->value.identifier) free(expr->value.identifier);
            break;
        case OP_STRING:
            if (expr->value.string) free(expr->value.string);
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
        case NODE_SELECT:
            if (node->value.select.columns) {
                for (int i = 0; i < node->value.select.column_count; i++) {
                    free(node->value.select.columns[i]);
                }
                free(node->value.select.columns);
            }
            if (node->value.select.table) free(node->value.select.table);
            expr_destroy(node->value.select.where);
            break;
        case NODE_INSERT:
            if (node->value.insert.table) free(node->value.insert.table);
            if (node->value.insert.columns) {
                for (int i = 0; i < node->value.insert.column_count; i++) {
                    free(node->value.insert.columns[i]);
                }
                free(node->value.insert.columns);
            }
            if (node->value.insert.values) {
                for (int i = 0; i < node->value.insert.value_count; i++) {
                    expr_destroy(node->value.insert.values[i]);
                }
                free(node->value.insert.values);
            }
            break;
        case NODE_DELETE:
            if (node->value.delete.table) free(node->value.delete.table);
            expr_destroy(node->value.delete.where);
            break;
        case NODE_UPDATE:
            if (node->value.update.table) free(node->value.update.table);
            if (node->value.update.columns) {
                for (int i = 0; i < node->value.update.set_count; i++) {
                    free(node->value.update.columns[i]);
                }
                free(node->value.update.columns);
            }
            if (node->value.update.values) {
                for (int i = 0; i < node->value.update.set_count; i++) {
                    expr_destroy(node->value.update.values[i]);
                }
                free(node->value.update.values);
            }
            expr_destroy(node->value.update.where);
            break;
        case NODE_IDENTIFIER:
            if (node->value.identifier) free(node->value.identifier);
            break;
        case NODE_STRING:
            if (node->value.string) free(node->value.string);
            break;
        default:
            break;
    }

    free(node);
}
