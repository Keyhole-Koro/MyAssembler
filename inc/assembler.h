#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.h"
#include "parser.h"
#include "codeGen.h"
#include "utils.h"

// Assemble input file. With a report_path, also write the debug report there:
// the tokens and the parsed lines. NULL writes none.
MachineCode assembler(const char *file_path, const char *report_path);

// Optional: write a MyCCLinker-compatible .obj file from assembled output.
// Returns 1 on success, 0 if the file could not be opened or written.
int write_object(const char *obj_path, const MachineCode *mc);

// Write direct source dependencies recorded by assembly `import ... from` lines.
// Paths are canonicalized relative to file_path.
int write_dependency_file(const char *depfile_path, const char *file_path);

#endif // ASSEMBLER_H
