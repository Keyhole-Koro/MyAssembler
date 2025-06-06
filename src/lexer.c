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
bool is_alphaOrUnderbar(char c) {
    return is_alpha(c) || c == '_';
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
        "MOV", "MOVI", "LD", "ST",

        // ➕ Arithmetic/Logic
        "ADD", "SUB", "CMP", "AND", "OR", "XOR", "SHL", "SHR",

        // 🔁 Control Flow
        "JMP", "JZ", "JNZ", "CALL", "RET",

        // 📦 Stack
        "PUSH", "POP",

        // 🌐 I/O
        "IN", "OUT",

        // 🛑 Special
        "HALT"
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


Token *lexer(const char *ptr, Token **head, Token *cur) {

    if (*head == NULL) {
        *head = (Token *)calloc(1, sizeof(Token));
        cur = *head;
    } 
    
    while (*ptr) {
        
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
            char buffer[MAX_TOKEN_LEN];
            buffer[0] = '0';
            buffer[1] = 'x';
            readUntil(buffer + 2, MAX_TOKEN_LEN - 2, ptr + 2, is_number);
        
            long value = strtol(buffer, NULL, 16);  // Convert hex string to integer
            
            cur = create_token(cur, NUMBER, itostr(value));  // Store numeric value as string
            ptr += strlen(buffer);
            continue;
        }        

        if (is_number(*ptr)) {
            char buffer[MAX_TOKEN_LEN];
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_number);
            cur = create_token(cur, NUMBER, buffer);
            ptr += strlen(buffer);
            continue;
        } 

        if (*ptr == 'R' && is_number(*(ptr + 1))) {
            char buffer[MAX_TOKEN_LEN];
            buffer[0] = 'R';
            readUntil(buffer + 1, MAX_TOKEN_LEN - 1, ptr + 1, is_number);
            cur = create_token(cur, REGISTER, buffer);
            ptr += strlen(buffer);
            continue;
        }


        if (is_alpha(*ptr)) {

            Token *inst = instruction(cur, ptr);
            if (inst) {
                cur = inst;
                ptr += strlen(cur->str);
                continue;
            }{
            
            char buffer[MAX_TOKEN_LEN];
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_alphaOrUnderbar);
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
