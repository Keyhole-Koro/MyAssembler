#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

const char* token_type_to_string(TokenType type);

void print_binary32(uint32_t value);

// Forward decl to avoid heavy includes here
typedef struct LabelInstructionLine LabelInstructionLine;

// Pretty-print the parsed program (labels, instructions, data) to console
void print_label_instruction_lines(const LabelInstructionLine *head);

// Write the parsed program (labels, instructions, data) to a file
void fprint_label_instruction_lines(FILE *out, const LabelInstructionLine *head);

// Dump token stream helpers
void print_tokens(const Token *tokens);
void fprint_tokens(FILE *out, const Token *tokens);

// Convenience: write full debug report (tokens + parsed program) to path
void write_debug_report(const char *path, const Token *tokens, const LabelInstructionLine *program);

#endif
