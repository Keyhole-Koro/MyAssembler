#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.h"
#include "parser.h"
#include "codeGen.h"
#include "utils.h"

// Assemble input file. Use output_path (binary path) to place debug report
// alongside it, named with the input file's stem, suffixed with "_asm.txt".
MachineCode assembler(const char *file_path, const char *output_path);

// Optional: write a MyCCLinker-compatible .obj file from assembled output.
void write_object(const char *obj_path, const MachineCode *mc);

// Write direct source dependencies recorded by assembly `import ... from` lines.
// Paths are canonicalized relative to file_path.
int write_dependency_file(const char *depfile_path, const char *file_path);

#endif // ASSEMBLER_H
