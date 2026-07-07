#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 关键字查表结构，替代一堆if判断，方便后续加关键字
typedef struct {
    const char* name;
    TokenType   type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"SELECT", TK_SELECT},
    {"FROM",   TK_FROM},
    {"WHERE",  TK_WHERE},
    {"INSERT", TK_INSERT},
    {"INTO",   TK_INTO},
    {"DELETE", TK_DELETE},
    {"UPDATE", TK_UPDATE},
    {"SET",    TK_SET},
    {"VALUES", TK_VALUES},
    {"AND",    TK_AND},
    {"OR",     TK_OR},
    {NULL,     TK_EOF}
};

static char lexer_peek(const Lexer* lexer)
{
    return *lexer->current;
}

static char lexer_next(Lexer* lexer)
{
    char c = *lexer->current;
    if (c == '\n')
    {
        lexer->line++;
        lexer->column = 1;
    }
    else
    {
        lexer->column++;
    }
    lexer->current++;
    return c;
}

static int lexer_eof(const Lexer* lexer)
{
    return *lexer->current == '\0';
}

static void lexer_skip_whitespace(Lexer* lexer)
{
    while (!lexer_eof(lexer) && isspace((unsigned char)lexer_peek(lexer)))
    {
        lexer_next(lexer);
    }
}

// 统一token创建，malloc失败返回NULL
static Token* create_token(const Lexer* lexer, TokenType type, const char* value)
{
    Token* token = malloc(sizeof(Token));
    if (!token) return NULL;

    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = lexer->line;
    token->column = lexer->column;
    return token;
}

// 通过查表识别关键字，无匹配返回TK_IDENTIFIER
static TokenType lookup_keyword(const char* buf)
{
    for (int i = 0; keywords[i].name != NULL; i++)
    {
        if (strcmp(buf, keywords[i].name) == 0)
        {
            return keywords[i].type;
        }
    }
    return TK_IDENTIFIER;
}

static Token* lexer_read_identifier(Lexer* lexer)
{
    const char* start = lexer->current;
    while (!lexer_eof(lexer) && (isalnum((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '_'))
    {
        lexer_next(lexer);
    }

    ptrdiff_t len = lexer->current - start;
    char* value = malloc((size_t)len + 1);
    if (!value) return create_token(lexer, TK_EOF, NULL);

    memcpy(value, start, (size_t)len);
    value[len] = '\0';

    TokenType type = lookup_keyword(value);
    Token* tok;
    if (type != TK_IDENTIFIER)
    {
        free(value); // 关键字不需要存储value，释放避免泄漏
        tok = create_token(lexer, type, NULL);
    }
    else
    {
        tok = create_token(lexer, type, value);
        free(value); // create_token内部strdup复制一份，本地缓冲区可释放
    }
    return tok;
}

static Token* lexer_read_integer(Lexer* lexer)
{
    const char* start = lexer->current;
    while (!lexer_eof(lexer) && isdigit((unsigned char)lexer_peek(lexer)))
    {
        lexer_next(lexer);
    }

    ptrdiff_t len = lexer->current - start;
    char* buf = malloc((size_t)len + 1);
    if (!buf) return create_token(lexer, TK_EOF, NULL);

    memcpy(buf, start, (size_t)len);
    buf[len] = '\0';

    Token* tok = create_token(lexer, TK_INTEGER, buf);
    free(buf);
    return tok;
}

static Token* lexer_read_string(Lexer* lexer)
{
    lexer_next(lexer); // 跳过开头单引号
    const char* start = lexer->current;

    while (!lexer_eof(lexer) && lexer_peek(lexer) != '\'')
    {
        if (lexer_peek(lexer) == '\\')
        {
            lexer_next(lexer); // 跳过转义符本身
            if (!lexer_eof(lexer))
                lexer_next(lexer); // 跳过转义后的字符
        }
        else
        {
            lexer_next(lexer);
        }
    }

    // 有闭合引号则跳过
    if (!lexer_eof(lexer))
        lexer_next(lexer);

    ptrdiff_t len = lexer->current - start - 1;
    char* buf = malloc((size_t)len + 1);
    if (!buf) return create_token(lexer, TK_EOF, NULL);

    memcpy(buf, start, (size_t)len);
    buf[len] = '\0';

    Token* tok = create_token(lexer, TK_STRING, buf);
    free(buf);
    return tok;
}

Lexer* lexer_init(const char* input)
{
    if (!input) return NULL;
    Lexer* lexer = malloc(sizeof(Lexer));
    if (!lexer) return NULL;

    lexer->input = input;
    lexer->current = input;
    lexer->line = 1;
    lexer->column = 1;
    return lexer;
}

void lexer_destroy(Lexer* lexer)
{
    free(lexer); // free空指针安全，无需if判断
}

Token* lexer_next_token(Lexer* lexer)
{
    if (!lexer) return NULL;
    lexer_skip_whitespace(lexer);

    if (lexer_eof(lexer))
    {
        return create_token(lexer, TK_EOF, NULL);
    }

    char c = lexer_peek(lexer);

    if (isalpha((unsigned char)c) || c == '_')
    {
        return lexer_read_identifier(lexer);
    }
    if (isdigit((unsigned char)c))
    {
        return lexer_read_integer(lexer);
    }
    if (c == '\'')
    {
        return lexer_read_string(lexer);
    }

    switch (c)
    {
        case '=':
            lexer_next(lexer);
            return create_token(lexer, TK_EQ, NULL);
        case '!':
            lexer_next(lexer);
            if (!lexer_eof(lexer) && lexer_peek(lexer) == '=')
            {
                lexer_next(lexer);
                return create_token(lexer, TK_NE, NULL);
            }
            break; // 单独!未处理，落到下方非法字符
        case '<':
            lexer_next(lexer);
            if (!lexer_eof(lexer) && lexer_peek(lexer) == '=')
            {
                lexer_next(lexer);
                return create_token(lexer, TK_LE, NULL);
            }
            return create_token(lexer, TK_LT, NULL);
        case '>':
            lexer_next(lexer);
            if (!lexer_eof(lexer) && lexer_peek(lexer) == '=')
            {
                lexer_next(lexer);
                return create_token(lexer, TK_GE, NULL);
            }
            return create_token(lexer, TK_GT, NULL);
        case ',': lexer_next(lexer); return create_token(lexer, TK_COMMA, NULL);
        case ';': lexer_next(lexer); return create_token(lexer, TK_SEMICOLON, NULL);
        case '(': lexer_next(lexer); return create_token(lexer, TK_LPAREN, NULL);
        case ')': lexer_next(lexer); return create_token(lexer, TK_RPAREN, NULL);
        case '*': lexer_next(lexer); return create_token(lexer, TK_ASTERISK, NULL);
        case '-': lexer_next(lexer); return create_token(lexer, TK_MINUS, NULL);
    }

    // 非法字符修复：不再错误返回IDENTIFIER，新增TK_BAD_TOKEN
    char bad[2];
    bad[0] = lexer_next(lexer);
    bad[1] = '\0';
    fprintf(stderr, "[Lexer Error] Line %d Col %d: unknown char '%c'\n",
            lexer->line, lexer->column - 1, bad[0]);
    return create_token(lexer, TK_BAD_TOKEN, bad);
}

void token_destroy(Token* token)
{
    if (token)
    {
        free(token->value); // free(NULL)安全，省去if判断
        free(token);
    }
}

const char* token_type_to_string(TokenType type)
{
    switch (type)
    {
        case TK_EOF:         return "TK_EOF";
        case TK_IDENTIFIER:  return "TK_IDENTIFIER";
        case TK_INTEGER:     return "TK_INTEGER";
        case TK_STRING:      return "TK_STRING";
        case TK_SELECT:      return "TK_SELECT";
        case TK_FROM:        return "TK_FROM";
        case TK_WHERE:       return "TK_WHERE";
        case TK_INSERT:      return "TK_INSERT";
        case TK_INTO:        return "TK_INTO";
        case TK_DELETE:      return "TK_DELETE";
        case TK_UPDATE:      return "TK_UPDATE";
        case TK_SET:         return "TK_SET";
        case TK_VALUES:      return "TK_VALUES";
        case TK_EQ:          return "TK_EQ";
        case TK_NE:          return "TK_NE";
        case TK_LT:          return "TK_LT";
        case TK_GT:          return "TK_GT";
        case TK_LE:          return "TK_LE";
        case TK_GE:          return "TK_GE";
        case TK_AND:         return "TK_AND";
        case TK_OR:          return "TK_OR";
        case TK_COMMA:       return "TK_COMMA";
        case TK_SEMICOLON:   return "TK_SEMICOLON";
        case TK_LPAREN:     return "TK_LPAREN";
        case TK_RPAREN:     return "TK_RPAREN";
        case TK_ASTERISK:   return "TK_ASTERISK";
        case TK_MINUS:      return "TK_MINUS";
        case TK_BAD_TOKEN:  return "TK_BAD_TOKEN";
        default:            return "TK_UNKNOWN";
    }
}
