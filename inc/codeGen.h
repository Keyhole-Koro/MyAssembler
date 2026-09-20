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
    uint32_t offset;      // offset to patch within `section`
    char *symbol_name;    // target symbol
    uint32_t type;        // 0=ABSOLUTE, 1=RELATIVE, 2=WORD32
    uint32_t section;     // 0=TEXT, 1=DATA, 2=COLLECT blob
} ObjReloc;

typedef struct {
    char *name;           // section name
    uint32_t offset;      // chunk offset in the collected-section blob
    uint32_t size;        // chunk size in bytes
} ObjCollect;

typedef struct {
    uint8_t *code; // raw bytes (text section)
    size_t size;   // number of bytes

    uint8_t *data;     // raw bytes (data section: `.data` blocks)
    size_t data_size;

    ObjSymbol *symbols;
    size_t symbol_count;

    ObjReloc *relocs;
    size_t reloc_count;

    // Chunks of named collected sections (see ObjectFormat.h CollectEntry),
    // whose bytes are in `blob`.
    ObjCollect *collects;
    size_t collect_count;
    uint8_t *blob;
    size_t blob_size;
} MachineCode;

// Encode a single data byte (mask to 8 bits)
uint8_t encodeByte(uint32_t value);

MachineCode codeGen(AsmBlock *head, const char **imports, size_t import_count, const char **exports, size_t export_count, const char *module_tag);

#endif
