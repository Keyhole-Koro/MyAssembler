#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "instructions.h"

#define MAX_TOKEN_LEN 128

bool is_number(char c) {
    return c >= '0' && c <= '9';
}

bool is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_hex(const char *ptr) {
    if (!ptr || ptr[0] != '0' || (ptr[1] != 'x' && ptr[1] != 'X')) {
        return false;
    }

    // Ensure at least one hex digit follows
    return isxdigit(ptr[2]);
}

bool is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

bool is_alphaOrUnderbar(char c) {
    return is_alpha(c) || c == '_';
}

bool is_alphaOrUnderbarOrNumber(char c) {
    return is_alpha(c) || c == '_' || is_number(c);
}

bool isTab(char c) {
    return c == '\t';
}

char* itostr(long value) {
    static char str[32];  // Static buffer, large enough for most integers
    snprintf(str, sizeof(str), "%ld", value);
    return str;
}

Token *create_token(Token *cur, TokenType type, const char *str, int line, int col) {
    Token *token = (Token *)calloc(1, sizeof(Token));
    token->type = type;
    token->str = (char *)calloc(strlen(str) + 1, 1);
    strcpy(token->str, str);
    token->line = line;
    token->col = col;
    cur->next = token;
    return token;
}

bool expect(char expected, char *ptr) {
    if (*ptr == expected) {
        return true;
    }
    return false;
}

bool expect_condition(bool (*condition)(char), char *ptr) {
    if (condition(*ptr)) {
        return true;
    }
    return false;
}

void readUntil(char *buffer, int max_len, const char *ptr, bool (*condition)(char)) {
    int index = 0;
    while (*ptr && condition(*ptr) && index < max_len - 1) {
        buffer[index++] = *ptr++;
    }
    buffer[index] = '\0';
}

// slow
Token *instruction(Token *cur, const char *ptr, int line, int col) {
    // Match against canonical instruction table (lowercase, case-sensitive)
    for (size_t i = 0; i < instruction_count; i++) {
        const char *name = instruction_table[i].name;
        size_t len = strlen(name);
        if (strncmp(ptr, name, len) == 0) {
            char next = ptr[len];
            if (next == '\0' || (!isalnum((unsigned char)next) && next != '_')) {
                return create_token(cur, INSTRUCTION, name, line, col);
            }
        }
    }
    return NULL;
}

//slow
bool is_register(const char *reg, char *buffer) {
    const char *registers[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "pc", "sp", "bp", "sr", "lr"
    };
    int count = sizeof(registers) / sizeof(registers[0]);

    for (int i = 0; i < count; i++) {
        size_t len = strlen(registers[i]);
        if (strncmp(reg, registers[i], len) == 0) {
            char next = reg[len];
            if (next == '\0' || (!isalnum(next) && next != '_')) {
                strncpy(buffer, registers[i], len + 1);
                return true;
            }
        }
    }
    return false;
}

static void read_identifier(const char **ptr, char *buffer, int *col) {
    int idx = 0;
    while (**ptr && is_alphaOrUnderbarOrNumber(**ptr)) {
        if (idx < MAX_TOKEN_LEN - 1) buffer[idx++] = **ptr;
        (*ptr)++;
        (*col)++;
    }
    buffer[idx] = '\0';
}

Token *lexer(const char *ptr, Token **head, Token *cur, int line) {

    if (*head == NULL) {
        *head = (Token *)calloc(1, sizeof(Token));
        cur = *head;
    } 

    char buffer[MAX_TOKEN_LEN];
    int col = 1;
    
    while (*ptr) {
        buffer[0] = '\0';  // Reset buffer

        if (*ptr == '\n') {
            int tok_line = line, tok_col = col;
            cur = create_token(cur, NEWLINE, "\n", tok_line, tok_col);
            col = 1;
            ptr++;
            continue;
        }

        if (isspace(*ptr)) {
            col++;
            ptr++;
            continue;
        }

        if (isTab(*ptr)) {
            col++;
            ptr++;
            continue;
        }

        int tok_line = line;
        int tok_col = col;

        if (*ptr == ':') {
            cur = create_token(cur, COLON, ":", tok_line, tok_col);
            col++;
            ptr++;
            continue;
        }

        if (*ptr == ';') {
            // Skip comments
            while (*ptr && *ptr != '\n') {
                col++;
                ptr++;
            }
            continue;
        }
        
        if (*ptr == '.') {
            cur = create_token(cur, PERIOD, ".", tok_line, tok_col);
            col++;
            ptr++;
            continue;
        }

        if (*ptr == ',') {
            cur = create_token(cur, COMMA, ",", tok_line, tok_col);
            col++;
            ptr++;
            continue;
        }

        if (*ptr == '{') {
            cur = create_token(cur, L_BRACE, "{", tok_line, tok_col);
            col++;
            ptr++;
            continue;
        }

        if (*ptr == '}') {
            cur = create_token(cur, R_BRACE, "}", tok_line, tok_col);
            col++;
            ptr++;
            continue;
        }

        if (*ptr == '"') {
            ptr++;
            col++;
            int idx = 0;
            while (*ptr && *ptr != '"') {
                if (idx < MAX_TOKEN_LEN - 1) buffer[idx++] = *ptr;
                ptr++;
                col++;
            }
            buffer[idx] = '\0';
            if (*ptr != '"') {
                fprintf(stderr, "Unterminated string literal at line %d col %d\n", tok_line, tok_col);
                exit(EXIT_FAILURE);
            }
            ptr++;
            col++;
            cur = create_token(cur, STRING_LITERAL, buffer, tok_line, tok_col);
            continue;
        }

        if (is_hex(ptr)) {
            buffer[0] = '0';
            buffer[1] = 'x';
            readUntil(buffer + 2, MAX_TOKEN_LEN - 2, ptr + 2, is_hex_digit);
        
            long value = strtol(buffer, NULL, 16);  // Convert hex string to integer
            
            cur = create_token(cur, NUMBER, itostr(value), tok_line, tok_col);  // Store numeric value as string
            ptr += strlen(buffer);
            col += (int)strlen(buffer);
            continue;
        }        

        if (is_number(*ptr)) {
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_number);
            cur = create_token(cur, NUMBER, buffer, tok_line, tok_col);
            ptr += strlen(buffer);
            col += (int)strlen(buffer);
            continue;
        }

        if (*ptr == '-' && is_number(*(ptr + 1))) {
            readUntil(buffer, MAX_TOKEN_LEN, ptr + 1, is_number);
            long value = strtol(buffer, NULL, 10);  // Convert string to integer
            snprintf(buffer, MAX_TOKEN_LEN, "-%ld", value);  // Add negative sign
            cur = create_token(cur, NEGATIVE_NUMBER, buffer, tok_line, tok_col);
            ptr += strlen(buffer) + 1;  // Move past the number
            col += (int)strlen(buffer) + 1;
            continue;
        }

        if (is_register(ptr, buffer)) {
            cur = create_token(cur, REGISTER, buffer, tok_line, tok_col);
            ptr += strlen(buffer);
            col += (int)strlen(buffer);
            continue;
        }

        Token *instTok = instruction(cur, ptr, tok_line, tok_col);
        if (instTok) {
            cur = instTok;
            col += (int)strlen(instTok->str);
            ptr += strlen(instTok->str);
            continue;
        }

        if (is_alphaOrUnderbar(*ptr)) {
            read_identifier(&ptr, buffer, &col);
            if (strcmp(buffer, "import") == 0) {
                cur = create_token(cur, IMPORT, buffer, tok_line, tok_col);
            } else if (strcmp(buffer, "from") == 0) {
                cur = create_token(cur, FROM, buffer, tok_line, tok_col);
            } else {
                cur = create_token(cur, LABEL, buffer, tok_line, tok_col);
            }
            continue;
        }

        fprintf(stderr, "Unknown character at line %d col %d: %c\n", line, col, *ptr);
        exit(EXIT_FAILURE);
    }

    return cur;
}
