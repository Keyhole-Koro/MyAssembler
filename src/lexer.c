#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 20

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

Token *create_token(Token *cur, TokenType type, const char *str) {
    Token *token = (Token *)calloc(1, sizeof(Token));
    token->type = type;
    token->str = (char *)calloc(strlen(str) + 1, 1);
    strcpy(token->str, str);
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
Token *instruction(Token *cur, const char *ptr) {
    const char *instructions[] = {
        // 📥 Data Movement
        "mov", "movi", "movis", "load", "store",

        // ➕ Arithmetic/Logic
        "add", "sub", "cmp", "and", "or", "xor", "shl", "shr",

        // 🔁 Control Flow
        "jmp", "jz", "jnz", "jg", "jl", "ja", "jb",

        // 📦 Stack
        "push", "pop",

        // 🌐 I/O
        "in", "out",

        // 🛑 Special
        "halt"
    };

    for (int i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
        size_t len = strlen(instructions[i]);
        if (strncmp(ptr, instructions[i], len) == 0) {
            char next = ptr[len];
            if (next == '\0' || !isalnum(next) && next != '_') {
                return create_token(cur, INSTRUCTION, instructions[i]);
            }
        }
    }
    return NULL;
}

//slow
bool is_register(const char *reg, char *buffer) {
    const char *registers[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "pc", "sp", "bp", "sr", "ir"
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

Token *lexer(const char *ptr, Token **head, Token *cur) {

    if (*head == NULL) {
        *head = (Token *)calloc(1, sizeof(Token));
        cur = *head;
    } 

    char buffer[MAX_TOKEN_LEN];
    
    while (*ptr) {
        buffer[0] = '\0';  // Reset buffer
        if (isspace(*ptr)) {
            ptr++;
            continue;
        }

        if (isTab(*ptr)) {
            ptr++;
            continue;
        }

        if (*ptr == '\n') {
            cur = create_token(cur, NEWLINE, "\n");
            ptr++;
            continue;
        }

        if (*ptr == ':') {
            cur = create_token(cur, COLON, ":");
            ptr++;
            continue;
        }

        if (*ptr == ';') {
            // Skip comments
            while (*ptr && *ptr != '\n') {
                ptr++;
            }
            continue;
        }
        
        if (*ptr == '.') {
            cur = create_token(cur, PERIOD, ".");
            ptr++;
            continue;
        }

        if (*ptr == ',') {
            cur = create_token(cur, COMMA, ",");
            ptr++;
            continue;
        }

        if (is_hex(ptr)) {
            buffer[0] = '0';
            buffer[1] = 'x';
            readUntil(buffer + 2, MAX_TOKEN_LEN - 2, ptr + 2, is_hex_digit);
        
            long value = strtol(buffer, NULL, 16);  // Convert hex string to integer
            
            cur = create_token(cur, NUMBER, itostr(value));  // Store numeric value as string
            ptr += strlen(buffer);
            continue;
        }        

        if (is_number(*ptr)) {
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_number);
            cur = create_token(cur, NUMBER, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (*ptr == '-' && is_number(*(ptr + 1))) {
            readUntil(buffer, MAX_TOKEN_LEN, ptr + 1, is_number);
            long value = strtol(buffer, NULL, 10);  // Convert string to integer
            snprintf(buffer, MAX_TOKEN_LEN, "-%ld", value);  // Add negative sign
            cur = create_token(cur, NEGATIVE_NUMBER, buffer);
            ptr += strlen(buffer) + 1;  // Move past the number
            continue;
        }

        if (is_register(ptr, buffer)) {
            cur = create_token(cur, REGISTER, buffer);
            ptr += strlen(buffer);
            continue;
        }

        if (is_alpha(*ptr) || *ptr == '_') {

            Token *inst = instruction(cur, ptr);
            if (inst) {
                cur = inst;
                ptr += strlen(cur->str);
                continue;
            }{
            
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_alphaOrUnderbarOrNumber);
            cur = create_token(cur, LABEL, buffer);
            ptr += strlen(buffer);
            continue;
        }

        fprintf(stderr, "Error: Unrecognized character '%c'\n", *ptr);
        exit(EXIT_FAILURE);
        }
    }
    return cur;
}
