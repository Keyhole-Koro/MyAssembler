#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "assembler.h"

static void write_hex_dump(const char *bin_path, const uint8_t *buf, size_t size) {
    if (!buf || size == 0) return;

    // Derive build/<stem>.txt from the provided binary path
    const char *slash1 = strrchr(bin_path, '/');
    const char *slash2 = strrchr(bin_path, '\\');
    const char *base = bin_path;
    if (slash1 && slash2) base = (slash1 > slash2) ? slash1 + 1 : slash2 + 1;
    else if (slash1) base = slash1 + 1;
    else if (slash2) base = slash2 + 1;

    char stem[256];
    size_t blen = strlen(base);
    if (blen >= sizeof(stem)) blen = sizeof(stem) - 1;
    memcpy(stem, base, blen);
    stem[blen] = '\0';
    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';

    char out_path[512];
    snprintf(out_path, sizeof(out_path), "%s.txt", stem[0] ? stem : "output");

    FILE *txt = fopen(out_path, "w");
    if (!txt) {
        perror("Failed to open hex dump file");
        return;
    }

    // Headings: column indices
    fprintf(txt, "Offset    ");
    for (int c = 0; c < 16; ++c) {
        fprintf(txt, "%02X%s", c, (c == 15 ? "\n" : " "));
    }

    // Rows: 16 bytes per row with offset label
    size_t i = 0;
    while (i < size) {
        unsigned long long off = (unsigned long long)i;
        fprintf(txt, "%08llX: ", off);
        int col = 0;
        for (; col < 16 && i < size; ++col, ++i) {
            fprintf(txt, "%02X", (unsigned)buf[i]);
            if (col != 15 && i != size) fputc(' ', txt);
        }
        // If short row, still end line
        fputc('\n', txt);
    }

    fclose(txt);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.bin>\n", argv[0]);
        return 1;
    }
    const char *input_path = argv[1];
    const char *output_path = argv[2];

    MachineCode mc = assembler(input_path, output_path);

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Failed to open output file");
        free(mc.code);
        exit(EXIT_FAILURE);
    }

    fwrite(mc.code, 1, mc.size, out);
    fclose(out);

    // Also write a hex dump to <output-stem>.txt
    write_hex_dump(output_path, mc.code, mc.size);
    free(mc.code);

    return 0;
}
