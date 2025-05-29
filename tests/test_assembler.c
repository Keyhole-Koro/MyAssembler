#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assembler.h"

void test_assembler() {
    // Example test case for the assembler
    const char *input = "MOV R1, R2"; // Sample assembly instruction
    const char *expected_output = "0001 0010"; // Expected machine code output

    char *output = assemble(input);
    
    if (strcmp(output, expected_output) == 0) {
        printf("Test passed: %s -> %s\n", input, output);
    } else {
        printf("Test failed: %s -> %s (expected %s)\n", input, output, expected_output);
    }

    free(output);
}

int main() {
    test_assembler();
    return 0;
}