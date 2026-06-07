#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdint.h>
#include <stdlib.h>

#include "parser.h"
#include "lexer.h"
#include "utils.h"

typedef struct {
    char *name;
    uint32_t type;    // 0=UNDEFINED, 1=DEFINED
    uint32_t section; // 0=TEXT, 1=DATA
    uint32_t offset;
} ObjSymbol;

typedef struct {
    uint32_t offset;      // offset in text section to patch
    char *symbol_name;    // target symbol
    uint32_t type;        // 0=ABSOLUTE, 1=RELATIVE
} ObjReloc;

typedef struct {
    uint8_t *code; // raw bytes (text section)
    size_t size;   // number of bytes

    ObjSymbol *symbols;
    size_t symbol_count;

    ObjReloc *relocs;
    size_t reloc_count;
} MachineCode;

// Encode a single data byte (mask to 8 bits)
uint8_t encodeByte(uint32_t value);

MachineCode codeGen(AsmBlock *head, const char **imports, size_t import_count, const char **exports, size_t export_count, const char *module_tag);

#endif
