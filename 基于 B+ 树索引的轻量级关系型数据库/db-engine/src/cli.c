#include "database.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 4096

static void print_help() {
    printf("Available commands:\n");
    printf("  .help          Show this help message\n");
    printf("  .tables        List all tables\n");
    printf("  .exit          Exit the database shell\n");
    printf("  BEGIN          Start a transaction\n");
    printf("  COMMIT         Commit a transaction\n");
    printf("  ROLLBACK       Rollback a transaction\n");
    printf("  SQL statements are executed directly\n");
    printf("\nExample:\n");
    printf("  CREATE TABLE users (id INT, name TEXT);\n");
    printf("  INSERT INTO users VALUES (1, 'Alice');\n");
    printf("  SELECT * FROM users;\n");
}

static void print_tables(Database* db) {
    Executor* executor = db->executor;
    if (executor->table_count == 0) {
        printf("No tables found.\n");
        return;
    }
    
    printf("Tables (%d):\n", executor->table_count);
    for (int i = 0; i < executor->table_count; i++) {
        Table* table = &executor->tables[i];
        printf("  %s (records: %d", table->table_name, table->record_count);
        if (table->column_count > 0) {
            printf(", columns: ");
            for (int j = 0; j < table->column_count; j++) {
                if (j > 0) printf(", ");
                printf("%s", table->columns[j].name);
            }
        }
        printf(")\n");
    }
}

static void print_result_set(ResultSet* rs) {
    if (rs == NULL || rs->row_count == 0) {
        printf("No results.\n");
        return;
    }
    
    for (int i = 0; i < rs->column_count; i++) {
        if (i > 0) printf(" | ");
        printf("%s", rs->columns[i]);
    }
    printf("\n");
    
    for (int i = 0; i < rs->column_count; i++) {
        if (i > 0) printf("---+---");
        printf("------");
    }
    printf("\n");
    
    for (int i = 0; i < rs->row_count; i++) {
        printf("%s\n", rs->rows[i]);
    }
    
    printf("\n%d rows returned.\n", rs->row_count);
}

static int process_command(Database* db, const char* input) {
    if (input == NULL || input[0] == '\0') return 0;
    
    size_t len = strlen(input);
    char* cmd = (char*)malloc(len + 1);
    strcpy(cmd, input);
    
    if (cmd[len - 1] == '\n') cmd[len - 1] = '\0';
    
    if (strcmp(cmd, ".help") == 0) {
        print_help();
        free(cmd);
        return 0;
    }
    
    if (strcmp(cmd, ".tables") == 0) {
        print_tables(db);
        free(cmd);
        return 0;
    }
    
    if (strcmp(cmd, ".exit") == 0) {
        free(cmd);
        return 1;
    }
    
    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        free(cmd);
        return 1;
    }
    
    if (strcmp(cmd, "BEGIN") == 0 || strcmp(cmd, "begin") == 0) {
        if (db_begin_transaction(db) == 0) {
            printf("Transaction started.\n");
        } else {
            printf("Error: Failed to start transaction.\n");
        }
        free(cmd);
        return 0;
    }
    
    if (strcmp(cmd, "COMMIT") == 0 || strcmp(cmd, "commit") == 0) {
        if (db_commit(db) == 0) {
            printf("Transaction committed.\n");
        } else {
            printf("Error: Failed to commit transaction.\n");
        }
        free(cmd);
        return 0;
    }
    
    if (strcmp(cmd, "ROLLBACK") == 0 || strcmp(cmd, "rollback") == 0) {
        if (db_rollback(db) == 0) {
            printf("Transaction rolled back.\n");
        } else {
            printf("Error: Failed to rollback transaction.\n");
        }
        free(cmd);
        return 0;
    }
    
    ResultSet* rs = db_query(db, cmd);
    if (rs != NULL) {
        print_result_set(rs);
        result_set_destroy(rs);
    } else {
        printf("Query executed.\n");
    }
    
    free(cmd);
    return 0;
}

int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : "test.db";
    
    printf("B+ Tree Database Shell\n");
    printf("Type '.help' for help, '.exit' to quit.\n\n");
    
    Database* db = db_open(filename);
    if (db == NULL) {
        printf("Error: Failed to open database '%s'\n", filename);
        return 1;
    }
    
    char input[MAX_INPUT];
    int exit_flag = 0;
    
    while (!exit_flag) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, MAX_INPUT, stdin) == NULL) {
            break;
        }
        
        exit_flag = process_command(db, input);
        printf("\n");
    }
    
    db_close(db);
    printf("Goodbye!\n");
    
    return 0;
}
