#include <stdio.h>
#include <assert.h>
#include "codegen.h"

void test_codegen() {
    // Example test case for code generation
    const char *assembly_input = "MOV R1, R2\nADD R1, R3";
    unsigned char expected_output[] = { /* expected machine code bytes */ };
    unsigned char actual_output[256]; // Adjust size as needed

    // Call the code generation function
    int result = generate_machine_code(assembly_input, actual_output);

    // Validate the result
    assert(result == 0); // Assuming 0 indicates success
    assert(memcmp(expected_output, actual_output, sizeof(expected_output)) == 0);
}

int main() {
    test_codegen();
    printf("All code generation tests passed!\n");
    return 0;
}