#ifndef LEXER_H
#define LEXER_H

typedef enum {
    COLON = 1,
    COMMA,
    PERIOD,
    IDENTIFIER,
    INSTRUCTION,
    REGISTER,
    NUMBER,
    LABEL,
    NEWLINE,
} TokenType;

typedef struct Token Token;
struct Token {
    TokenType type;
    char *str;
    Token *next;
};

Token *lexer(const char *ptr, Token **head, Token *cur);

#endif