#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "assembler.h"

// Write hex dump to either a user-supplied path or, by default, alongside the
// binary output inside the same directory (e.g., MyTester/outputs/<stem>.txt).
static void write_hex_dump(const char *bin_path, const char *custom_path, const uint8_t *buf, size_t size) {
    if (!buf || size == 0) return;

    char out_path[512];
    if (custom_path && custom_path[0]) {
        snprintf(out_path, sizeof(out_path), "%s", custom_path);
    } else {
        // <dir>/<stem>.txt from the binary's path
        char stem[256];
        path_stem(bin_path, stem, sizeof(stem));
        path_beside(bin_path, stem[0] ? stem : "output", ".txt", out_path, sizeof(out_path));
    }

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
        fprintf(stderr, "Usage: %s <input.asm> <output.bin> [hexdump.txt] [--obj <output.obj>] [--depfile <path>]\n", argv[0]);
        return 1;
    }
    const char *input_path = argv[1];
    const char *output_path = argv[2];
    const char *hexdump_path = NULL;
    const char *obj_path = NULL;
    const char *depfile_path = NULL;

    for (int i = 3; i < argc; ++i) {
        if (strcmp(argv[i], "--obj") == 0 && i + 1 < argc) {
            obj_path = argv[i + 1];
            ++i;
        } else if (strcmp(argv[i], "--depfile") == 0 && i + 1 < argc) {
            depfile_path = argv[i + 1];
            ++i;
        } else if (strcmp(argv[i], "--obj") == 0 || strcmp(argv[i], "--depfile") == 0) {
            fprintf(stderr, "%s requires a path\n", argv[i]);
            return 1;
        } else if (!hexdump_path) {
            hexdump_path = argv[i];
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
            return 1;
        }
    }

    MachineCode mc = assembler(input_path, output_path);

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror("Failed to open output file");
        free(mc.code);
        exit(EXIT_FAILURE);
    }

    fwrite(mc.code, 1, mc.size, out);
    fclose(out);

    // Also write a hex dump. If a path is provided, use it; otherwise default
    // to <output-dir>/<output-stem>.txt (e.g., MyTester/outputs/<stem>.txt).
    write_hex_dump(output_path, hexdump_path, mc.code, mc.size);

    // Optionally emit .obj for linker use
    if (obj_path) {
        write_object(obj_path, &mc);
    }

    int depfile_ok = !depfile_path || write_dependency_file(depfile_path, input_path);

    free(mc.code);
    for (size_t i = 0; i < mc.symbol_count; ++i) free(mc.symbols[i].name);
    free(mc.symbols);
    for (size_t i = 0; i < mc.reloc_count; ++i) free(mc.relocs[i].symbol_name);
    free(mc.relocs);

    return depfile_ok ? 0 : 1;
}
