#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"
#include "utils.h"

typedef struct {
    uint32_t *code;
    size_t size;
} MachineCode;

MachineCode codeGen(LabelInstructionLine *head);

#endif