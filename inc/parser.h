#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

#define MAX_LABEL_LEN 32

// Forward declaration for Encode (defined elsewhere)
typedef struct Encode Encode;

// Base instruction containing only opcode (6 bits in lower bits)
typedef struct {
    uint8_t opcode;   // 6 bits used in lower bits (0-5)
} InstructionBase;

// Pattern: reg, reg (e.g. MOV R1, R2)
typedef struct {
    uint8_t opcode;   // 6 bits
    uint8_t reg1;     // 5 bits
    uint8_t reg2;     // 5 bits
} InstrRegReg;

// Pattern: reg, imm21 (e.g. MOVI R1, imm)
typedef struct {
    uint8_t opcode;   // 6 bits
    uint8_t reg1;     // 5 bits
    uint32_t imm21;   // 21 bits (stored in 32 bits, lower bits used)
} InstrRegImm21;

typedef struct {
    uint8_t opcode;   // 6 bits
    char *label;
} InstrLabel;

// Pattern: reg (e.g. SHL R1)
typedef struct {
    uint8_t opcode;   // 6 bits
    uint8_t reg1;     // 5 bits
} InstrReg;

// Pattern: imm26 (e.g. JMP addr)
typedef struct {
    uint8_t opcode;   // 6 bits
    uint32_t imm26;   // 26 bits (stored in 32 bits)
} InstrImm26;

// Pattern: no operand (e.g. RET, HALT)
typedef struct {
    uint8_t opcode;   // 6 bits only
} InstrNoOperand;

// Union that covers all instruction patterns
typedef union {
    InstructionBase base;
    InstrRegReg regreg;
    InstrRegImm21 regimm21;
    InstrReg reg;
    InstrLabel label;
    InstrImm26 imm26;
    InstrNoOperand noOperand;
} Instruction;

typedef struct InstructionList InstructionList;
struct InstructionList {
    Instruction *instruction;
    
    // must be replaced/fixed later (e.g., relative address fixup)
    bool needs_fixup;  

    InstructionList *next; // Pointer to the next encoded instruction
};

// Instruction line containing optional label and encoded instruction
typedef struct LabelInstructionLine LabelInstructionLine;
struct LabelInstructionLine {
    char *label; // Empty string if no label
    InstructionList *inst_list;            // Pointer to encoded instruction data
    LabelInstructionLine *next; // Pointer to the next label instruction line
};

LabelInstructionLine *parser(Token *head);
#endif // PARSER_H
