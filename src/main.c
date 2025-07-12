#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "assembler.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.bin>\n", argv[0]);
        return 1;
    }
    const char *asm_path = argv[1];
    const char *bin_path = argv[2];

    MachineCode mc = assembler(asm_path);

    FILE *out = fopen(bin_path, "wb");
    if (!out) {
        perror("Failed to open output file");
        free(mc.code);
        exit(EXIT_FAILURE);
    }

    fwrite(mc.code, sizeof(uint32_t), mc.size, out);
    fclose(out);
    free(mc.code);

    return 0;
}
