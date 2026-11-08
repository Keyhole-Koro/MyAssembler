#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

int32_t sign_extend_26bit(uint32_t x);
int32_t substraction_26bit(uint32_t a, uint32_t b);

const char* token_type_to_string(TokenType type);

// The file name in `path`: whatever follows its last '/' or '\\'.
const char *path_basename(const char *path);

// The file name in `path` without its extension, cut to fit `size` first.
void path_stem(const char *path, char *out, size_t size);

// `<name><suffix>` in the directory of `beside` (none if it is NULL or has no
// directory), written to `out`.
void path_beside(const char *beside, const char *name, const char *suffix, char *out, size_t size);

// Forward decl to avoid heavy includes here
typedef struct AsmBlock AsmBlock;

// Write the parsed program (labels, instructions, data) to a file
void fprint_label_instruction_lines(FILE *out, const AsmBlock *head);

// Dump the token stream
void fprint_tokens(FILE *out, const Token *tokens);

// Convenience: write full debug report (tokens + parsed program) to path
void write_debug_report(const char *path, const Token *tokens, const AsmBlock *program);

#endif
