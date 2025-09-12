#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *_lexer(const char *file_path) {
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

MachineCode assembler(const char *file_path, const char *output_path) {
    Token *tokens = _lexer(file_path);
    if (!tokens) {
        fprintf(stderr, "No tokens found.\n");
        return (MachineCode){NULL, 0};
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

    return codeGen(parsed);
}
