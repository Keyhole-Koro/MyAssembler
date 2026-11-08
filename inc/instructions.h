#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include <stddef.h>

// Canonical instruction/mnemonic mapping
typedef struct {
    const char *name;   // lowercase mnemonic
    uint8_t opcode;     // 6-bit opcode value
} InstructionDef;

extern const InstructionDef instruction_table[];
extern const size_t instruction_count;

// Lookup helpers
uint8_t opcode_from_mnemonic(const char *name);   // exits on unknown mnemonic
const InstructionDef* find_instruction(const char *name); // NULL if not found

// Register names (lowercase), indexed by the number an instruction encodes.
extern const char *const register_names[];
extern const size_t register_count;

// The number of register `name`, or -1 if it names none.
int register_number(const char *name);

#endif // INSTRUCTIONS_H

