#include "database.h"
#include "executor.h"
#include "parser.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>

Database* db_open(const char* filename) {
    Database* db = (Database*)malloc(sizeof(Database));
    db->executor = executor_init();
    db->filename = filename ? strdup(filename) : NULL;
    return db;
}

ResultSet* db_query(Database* db, const char* sql) {
    if (db == NULL || sql == NULL) return NULL;
    
    Lexer* lexer = lexer_init(sql);
    if (lexer == NULL) return NULL;
    
    Parser* parser = parser_init(lexer);
    if (parser == NULL) {
        lexer_destroy(lexer);
        return NULL;
    }
    
    ASTNode* node = parser_parse(parser);
    if (node == NULL) {
        parser_destroy(parser);
        lexer_destroy(lexer);
        return NULL;
    }
    
    ResultSet* rs = NULL;
    
    switch (node->type) {
        case NODE_SELECT:
            rs = execute_select(db->executor, &node->value.select);
            break;
        case NODE_INSERT:
            execute_insert(db->executor, &node->value.insert);
            break;
        case NODE_DELETE:
            execute_delete(db->executor, &node->value.delete);
            break;
        case NODE_UPDATE:
            execute_update(db->executor, &node->value.update);
            break;
        default:
            break;
    }
    
    ast_destroy(node);
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    return rs;
}

void db_close(Database* db) {
    if (db == NULL) return;
    
    executor_destroy(db->executor);
    if (db->filename) free(db->filename);
    free(db);
}
