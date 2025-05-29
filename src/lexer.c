#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LEN 20

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool isTab(char c) {
    return c == '\t';
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

Token *reserved_word(Token *cur, const char *ptr) {
    const char *reserved_words[] = {
        "HALT", "IN", "OUT"
    };
    for (int i = 0; i < sizeof(reserved_words) / sizeof(reserved_words[0]); i++) {
        if (strncmp(ptr, reserved_words[i], strlen(reserved_words[i])) == 0) {
            return create_token(cur, HALT + i, reserved_words[i]);
        }
    }
    return NULL;
}

Token *instruction(Token *cur, const char *ptr) {
    const char *instructions[] = {
        "ADD", "SUB", "MUL", "DIV", "MOV", "CMP", "JMP", "JEQ", "JNE",
        "JGT", "JLT", "CALL", "RET", "PUSH", "POP"
    };
    for (int i = 0; i < sizeof(instructions) / sizeof(instructions[0]); i++) {
        if (strncmp(ptr, instructions[i], strlen(instructions[i])) == 0) {
            return create_token(cur, INSTRUCTION, instructions[i]);
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

        if (*ptr == ':') {
            cur = create_token(cur, COLON, ":");
            ptr++;
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

        if (is_digit(*ptr)) {
            char buffer[MAX_TOKEN_LEN];
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_digit);
            cur = create_token(cur, NUMBER, buffer);
            ptr += strlen(buffer);
            continue;
        } 

        if (*ptr == 'R' && is_digit(*(ptr + 1))) {
            char buffer[MAX_TOKEN_LEN];
            buffer[0] = 'R';
            readUntil(buffer + 1, MAX_TOKEN_LEN - 1, ptr + 1, is_digit);
            cur = create_token(cur, REGISTER, buffer);
            ptr += strlen(buffer);
            continue;
        }


        if (is_alpha(*ptr)) {
            Token *reserved = reserved_word(cur, ptr);
            if (reserved) {
                cur = reserved;
                ptr += strlen(cur->str);
                continue;
            }

            Token *inst = instruction(cur, ptr);
            if (inst) {
                cur = inst;
                ptr += strlen(cur->str);
                continue;
            }{
            
            char buffer[MAX_TOKEN_LEN];
            readUntil(buffer, MAX_TOKEN_LEN, ptr, is_alpha);
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
