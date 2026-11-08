#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

int32_t sign_extend_26bit(uint32_t x);
int32_t substraction_26bit(uint32_t a, uint32_t b);

const char* token_type_to_string(TokenType type);

// Forward decl to avoid heavy includes here
typedef struct AsmBlock AsmBlock;

// Write the parsed program (labels, instructions, data) to a file
void fprint_label_instruction_lines(FILE *out, const AsmBlock *head);

// Dump the token stream
void fprint_tokens(FILE *out, const Token *tokens);

// Convenience: write full debug report (tokens + parsed program) to path
void write_debug_report(const char *path, const Token *tokens, const AsmBlock *program);

#endif
