#include "instructions.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// Authoritative instruction set (lowercase names) derived from parser mapping
// Do not alter opcodes without coordinating downstream consumers.
const InstructionDef instruction_table[] = {
    // data movement
    {"mov",   0x01},
    {"movi",  0x02},
    {"movis", 0x18},
    {"load",  0x03},
    {"store", 0x04},
    {"loadb", 0x1C},
    {"storeb",0x1D},

    // arith/logic
    {"add",   0x05},
    {"addis", 0x19},
    {"sub",   0x06},
    {"cmp",   0x07},
    {"and",   0x08},
    {"or",    0x09},
    {"xor",   0x0A},
    {"shl",   0x0B},
    {"shr",   0x0C},

    // control flow
    {"jmp",   0x0D},
    {"jz",    0x0E},
    {"jnz",   0x0F},
    {"jg",    0x10},
    {"jl",    0x11},
    {"ja",    0x12},
    {"jb",    0x13},
    {"call",  0x1B},

    // stack
    {"push",  0x14},
    {"pop",   0x15},

    // io
    {"in",    0x16},
    {"out",   0x17},

    // special
    {"halt",  0x3F},
    {"debug", 0x1A},
};

const size_t instruction_count = sizeof(instruction_table) / sizeof(instruction_table[0]);

static int case_insensitive_equals(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

const InstructionDef* find_instruction(const char *name) {
    for (size_t i = 0; i < instruction_count; ++i) {
        if (case_insensitive_equals(name, instruction_table[i].name)) {
            return &instruction_table[i];
        }
    }
    return NULL;
}

uint8_t opcode_from_mnemonic(const char *name) {
    const InstructionDef *def = find_instruction(name);
    if (!def) {
        fprintf(stderr, "Unknown opcode: %s\n", name ? name : "(null)");
        exit(EXIT_FAILURE);
    }
    return def->opcode;
}
