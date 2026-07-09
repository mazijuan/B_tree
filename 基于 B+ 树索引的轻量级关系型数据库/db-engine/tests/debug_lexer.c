#include "../src/lexer.h"
#include <stdio.h>

int main() {
    const char* sql = "SELECT * FROM users WHERE id = 1;";
    Lexer* lexer = lexer_init(sql);
    
    printf("Input: %s\n\n", sql);
    printf("Tokens:\n");
    
    Token* t;
    int count = 0;
    while ((t = lexer_next_token(lexer))->type != TK_EOF) {
        printf("  %d: %s", count++, token_type_to_string(t->type));
        if (t->value) {
            printf(" ('%s')", t->value);
        }
        printf("\n");
        token_destroy(t);
    }
    printf("  %d: %s\n", count++, token_type_to_string(t->type));
    token_destroy(t);
    
    lexer_destroy(lexer);
    return 0;
}
