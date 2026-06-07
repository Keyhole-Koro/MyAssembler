#ifndef LEXER_H
#define LEXER_H

typedef enum {
    COLON = 1,
    COMMA,
    PERIOD,
    L_BRACE,
    R_BRACE,
    IMPORT,
    EXPORT,
    FROM,
    STRING_LITERAL,
    IDENTIFIER,
    INSTRUCTION,
    REGISTER,
    NUMBER,
    NEGATIVE_NUMBER,
    LABEL,
    NEWLINE,
} TokenType;

typedef struct Token Token;
struct Token {
    TokenType type;
    char *str;
    int line;
    int col;
    Token *next;
};

Token *lexer(const char *ptr, Token **head, Token *cur, int line);

#endif
