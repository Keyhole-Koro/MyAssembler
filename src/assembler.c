#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ObjectFormat.h"

Token *_lexer(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    Token *head = NULL;
    Token *cur = NULL;

    int line = 1;
    while (fgets(buffer, sizeof(buffer), file)) {
        cur = lexer(buffer, &head, cur, line);
        line++;
    }

    fclose(file);

    return head;
}

MachineCode assembler(const char *file_path, const char *output_path) {
    parser_set_source_file(file_path);
    Token *tokens = _lexer(file_path);
    if (!tokens) {
        fprintf(stderr, "No tokens found.\n");
        return (MachineCode){NULL, 0, NULL, 0, NULL, 0};
    }

    AsmBlock *parsed = parser(tokens);

    // Build debug report path: same directory as output_path, filename is
    // <input-stem>_asm.txt
    const char *slash1 = output_path ? strrchr(output_path, '/') : NULL;
    const char *slash2 = output_path ? strrchr(output_path, '\\') : NULL;
    const char *dir_end = NULL;
    if (slash1 && slash2) dir_end = (slash1 > slash2) ? slash1 : slash2;
    else if (slash1) dir_end = slash1;
    else if (slash2) dir_end = slash2;

    const char *in_base = file_path;
    const char *in_s1 = strrchr(file_path, '/');
    const char *in_s2 = strrchr(file_path, '\\');
    if (in_s1 && in_s2) in_base = (in_s1 > in_s2) ? in_s1 + 1 : in_s2 + 1;
    else if (in_s1) in_base = in_s1 + 1;
    else if (in_s2) in_base = in_s2 + 1;

    char stem[256];
    size_t blen = strlen(in_base);
    if (blen >= sizeof(stem)) blen = sizeof(stem) - 1;
    memcpy(stem, in_base, blen);
    stem[blen] = '\0';
    char *dot = strrchr(stem, '.');
    if (dot) *dot = '\0';

    char out_path_buf[1024];
    if (dir_end) {
        size_t dir_len = (size_t)(dir_end - output_path + 1); // include slash
        if (dir_len >= sizeof(out_path_buf)) dir_len = sizeof(out_path_buf) - 1;
        memcpy(out_path_buf, output_path, dir_len);
        out_path_buf[dir_len] = '\0';
        snprintf(out_path_buf + dir_len, sizeof(out_path_buf) - dir_len, "%s_asm.txt", stem);
    } else {
        snprintf(out_path_buf, sizeof(out_path_buf), "%s_asm.txt", stem);
    }

    write_debug_report(out_path_buf, tokens, parsed);

    size_t import_count = 0;
    const char **imports = parser_get_imports(&import_count);
    return codeGen(parsed, imports, import_count, file_path);
}

void write_object(const char *obj_path, const MachineCode *mc) {
    if (!obj_path || !mc) return;
    FILE *f = fopen(obj_path, "wb");
    if (!f) {
        perror("Failed to open .obj file");
        return;
    }

    struct FileHeader hdr = {0};
    hdr.magic = LINKER_MAGIC;
    hdr.text_size = (uint32_t)mc->size;
    hdr.data_size = 0;
    hdr.symtable_count = (uint32_t)mc->symbol_count;
    hdr.reloc_count = (uint32_t)mc->reloc_count;
    fwrite(&hdr, sizeof(hdr), 1, f);

    // text section
    fwrite(mc->code, 1, mc->size, f);

    // no data section (yet)

    // symbols
    for (size_t i = 0; i < mc->symbol_count; ++i) {
        struct SymbolEntry se = {0};
        const ObjSymbol *src = &mc->symbols[i];
        strncpy(se.name, src->name ? src->name : "", sizeof(se.name) - 1);
        se.type = src->type;
        se.section = src->section;
        se.offset = src->offset;
        fwrite(&se, sizeof(se), 1, f);
    }

    // relocs
    for (size_t i = 0; i < mc->reloc_count; ++i) {
        struct RelocEntry re = {0};
        const ObjReloc *src = &mc->relocs[i];
        re.offset = src->offset;
        strncpy(re.symbol_name, src->symbol_name ? src->symbol_name : "", sizeof(re.symbol_name) - 1);
        re.type = src->type;
        fwrite(&re, sizeof(re), 1, f);
    }

    fclose(f);
}
