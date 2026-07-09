#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdint.h>

typedef enum {
    NODE_SELECT,
    NODE_INSERT,
    NODE_DELETE,
    NODE_UPDATE,
    NODE_CREATE_TABLE,
    NODE_EXPR,
    NODE_IDENTIFIER,
    NODE_INTEGER,
    NODE_STRING,
    NODE_COMPARISON
} NodeType;

typedef enum {
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_IDENTIFIER,
    OP_INTEGER,
    OP_STRING
} ExprOp;

typedef struct Expr Expr;

struct Expr {
    ExprOp op;
    union {
        struct {
            Expr* left;
            Expr* right;
        } binary;
        struct {
            char* column;
            Expr* value;
        } comparison;
        char* identifier;
        int integer;
        char* string;
    } value;
};

typedef struct {
    char** columns;
    int column_count;
    char* table;
    Expr* where;
} SelectStmt;

typedef struct {
    char* table;
    char** columns;
    int column_count;
    Expr** values;
    int value_count;
} InsertStmt;

typedef struct {
    char* table;
    Expr* where;
} DeleteStmt;

typedef struct {
    char* table;
    char** columns;
    Expr** values;
    int set_count;
    Expr* where;
} UpdateStmt;

typedef enum {
    TYPE_INT,
    TYPE_VARCHAR,
    TYPE_TEXT
} ColumnType;

typedef struct {
    char* name;
    ColumnType type;
} ColumnDef;

typedef struct {
    char* table;
    ColumnDef* columns;
    int column_count;
} CreateTableStmt;

typedef struct ASTNode {
    NodeType type;
    union {
        SelectStmt select;
        InsertStmt insert;
        DeleteStmt delete;
        UpdateStmt update;
        CreateTableStmt create_table;
        Expr* expr;
        char* identifier;
        int integer;
        char* string;
    } value;
} ASTNode;

typedef struct {
    Lexer* lexer;
    Token* current_token;
} Parser;

Parser* parser_init(Lexer* lexer);
void parser_destroy(Parser* parser);
ASTNode* parser_parse(Parser* parser);
void ast_destroy(ASTNode* node);

#endif
