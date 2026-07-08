#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AST_SELECT,
    AST_INSERT,
    AST_DELETE,
    AST_UPDATE
} AstNodeType;

typedef enum {
    EXPR_LITERAL_INT,
    EXPR_LITERAL_STRING,
    EXPR_IDENTIFIER,
    EXPR_BINARY
} ExprType;

typedef struct Expr {
    ExprType type;
    union {
        int64_t int_value;
        char* string_value;
        char* identifier;
        struct {
            TokenType op;
            struct Expr* left;
            struct Expr* right;
        } binary;
    } value;
} Expr;

typedef struct {
    char* table_name;
    int select_all;
    char** columns;
    size_t column_count;
    Expr* where;
} SelectStmt;

typedef struct {
    char* table_name;
    char** columns;
    size_t column_count;
    Expr** values;
    size_t value_count;
} InsertStmt;

typedef struct {
    char* table_name;
    Expr* where;
} DeleteStmt;

typedef struct {
    char* table_name;
    char* column_name;
    Expr* value;
    Expr* where;
} UpdateStmt;

typedef struct AstNode {
    AstNodeType type;
    union {
        SelectStmt select;
        InsertStmt insert;
        DeleteStmt delete;
        UpdateStmt update;
    } value;
} AstNode;

AstNode* parse_sql(const char* sql);
void ast_destroy(AstNode* ast);
void expr_destroy(Expr* expr);

#endif
