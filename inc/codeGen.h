#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"
#include "utils.h"

typedef struct {
    uint8_t *code; // raw bytes
    size_t size;   // number of bytes
} MachineCode;

// Encode a single data byte (mask to 8 bits)
uint8_t encodeByte(uint32_t value);

MachineCode codeGen(AsmBlock *head);

#endif
