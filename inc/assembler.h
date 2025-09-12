#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.h"
#include "parser.h"
#include "codeGen.h"
#include "utils.h"

// Assemble input file. Use output_path (binary path) to place debug report
// alongside it, named with the input file's stem, suffixed with "_asm.txt".
MachineCode assembler(const char *file_path, const char *output_path);

#endif // ASSEMBLER_H
