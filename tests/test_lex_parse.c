#include "unity/unity.h"
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

Token *test_lexer(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    Token *head = NULL;
    Token *cur = NULL;

    while (fgets(buffer, sizeof(buffer), file)) {
        cur = lexer(buffer, &head, cur);
    }

    fclose(file);

    return head;
}

void setUp(void) {
    // This function is called before each test
}

void tearDown(void) {
    // This function is called after each test
}

void test_parser_sample1(void) {
    const char *file_path = "sample/label.masm";
    Token *tokens = test_lexer(file_path);

    TEST_ASSERT_NOT_NULL(tokens);

    LabelInstructionLine *parsed = parser(tokens);
    TEST_ASSERT_NOT_NULL(parsed);

    // Free tokens and parsed instructions
    Token *temp;
    while (tokens) {
        temp = tokens;
        tokens = tokens->next;
        free(temp->str);
        free(temp);
    }

    free(parsed);
}

void test_lexer_invalid_sample(void) {
    const char *file_path = "sample/invalid.masm";
    Token *tokens = test_lexer(file_path);

    TEST_ASSERT_NOT_NULL(tokens);

    // Check for invalid tokens
    Token *cur = tokens;
    while (cur) {
        if (cur->type == INSTRUCTION && strcmp(cur->str, "INVALID_OPCODE") == 0) {
            TEST_FAIL_MESSAGE("Lexer should not generate tokens for invalid opcodes.");
        }
        cur = cur->next;
    }

    // Free tokens
    Token *temp;
    while (tokens) {
        temp = tokens;
        tokens = tokens->next;
        free(temp->str);
        free(temp);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parser_sample1);
    RUN_TEST(test_lexer_invalid_sample);

    return UNITY_END();
}