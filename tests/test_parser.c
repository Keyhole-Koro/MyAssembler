#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

void test_parser_valid_input() {
    const char *input = "MOV R1, R2";
    ParserResult result = parse(input);
    // Check if the result is valid and matches expected output
    if (result.success) {
        printf("Test passed: Valid input parsed successfully.\n");
    } else {
        printf("Test failed: Valid input not parsed correctly.\n");
    }
}

void test_parser_invalid_input() {
    const char *input = "INVALID INSTRUCTION";
    ParserResult result = parse(input);
    // Check if the result indicates failure
    if (!result.success) {
        printf("Test passed: Invalid input correctly identified.\n");
    } else {
        printf("Test failed: Invalid input not identified correctly.\n");
    }
}

int main() {
    test_parser_valid_input();
    test_parser_invalid_input();
    return 0;
}