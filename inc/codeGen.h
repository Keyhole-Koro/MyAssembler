#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"

void *codeGen(LabelInstructionLine *head);

#endif