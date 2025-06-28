#include "assembler.h"

int main() {
    // Example usage of the assembler
    char *file_path = "tests/inputs/complex.masm";
    MachineCode mc = assembler(file_path);

        // Print and write to file
        for (size_t i = 0; i < mc.size; i++) {
            printf("----------------------------------\n");
            printf("machine code: %08X\n", mc.code[i]);
            printf("opcode: 0x%02X\n", (mc.code[i] >> 26) & 0x3F);
            print_binary32(mc.code[i]);
            printf("\n");
        }
    
        FILE *out = fopen("program.bin", "wb");
        if (!out) {
            perror("Failed to open output file");
            exit(EXIT_FAILURE);
        }
        fwrite(mc.code, sizeof(uint32_t), mc.size, out);
        fclose(out);
    
        free(mc.code);

    // You can add more tests or functionality here
    return 0;
}