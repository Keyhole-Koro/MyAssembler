#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.h"
#include "parser.h"
#include "codeGen.h"
#include "utils.h"

MachineCode assembler(const char *file_path);

#endif // ASSEMBLER_H

