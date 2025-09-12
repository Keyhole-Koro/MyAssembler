#include "codeGen.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define ENCODE(op, shift) ((uint32_t)(op) << (shift))

// A single label/address pair
typedef struct {
    char *label;
    uint32_t address;
} LabelAddressMapping;

// A dynamic list of mappings
typedef struct {
    LabelAddressMapping *entries;
    size_t count;
    size_t capacity;
} LabelMap;

void initLabelMap(LabelMap *map) {
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

void mapLabelToAddress(LabelMap *map, const char *label, uint32_t address) {
    // Resize if needed
    if (map->count == map->capacity) {
        size_t new_capacity = map->capacity == 0 ? 8 : map->capacity * 2;
        map->entries = realloc(map->entries, new_capacity * sizeof(LabelAddressMapping));
        if (!map->entries) {
            perror("Failed to allocate memory for label map");
            exit(EXIT_FAILURE);
        }
        map->capacity = new_capacity;
    }

    // Add new entry
    map->entries[map->count].label = strdup(label);
    map->entries[map->count].address = address;
    map->count++;
}

// Optional: Lookup function
int getLabelAddress(LabelMap *map, const char *label, uint32_t *out_address) {
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->entries[i].label, label) == 0) {
            *out_address = map->entries[i].address;
            return 1; // Found
        }
    }
    return 0; // Not found
}

uint8_t encodeByte(uint32_t value) {
    return (uint8_t)(value & 0xFFu);
}

uint32_t encodeRegReg(InstrRegReg instr) {
    assert(instr.reg1 < 32 && instr.reg2 < 32);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21) | ENCODE(instr.reg2, 16);
}

uint32_t encodeRegImm21(InstrRegImm21 instr) {
    assert(instr.reg1 < 32 && (instr.imm21 >> 21) == 0);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21) | ENCODE(instr.imm21 & 0x1FFFFF, 0);
}

uint32_t encodeReg(InstrReg instr) {
    assert(instr.reg1 < 32);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21);
}

uint32_t encodeLabel(LabelMap *labelMap, InstrLabel instr, uint32_t currentPC) {
    char *label = instr.label;
    uint32_t targetAddr = 0;

    if (getLabelAddress(labelMap, label, &targetAddr)) {

        int32_t offset = substraction_26bit(targetAddr, currentPC);

        if (offset < -(1 << 25) || offset >= (1 << 25)) {
            fprintf(stderr,
                "Error: Jump offset to label '%s' (%d bytes, aligned = %d) exceeds 26-bit signed range\n",
                label, offset >> 2, offset);
            exit(EXIT_FAILURE);
        }

        uint32_t offsetMasked = ((uint32_t)offset) & 0x03FFFFFF;

        uint32_t instruction = (instr.opcode << 26) | offsetMasked;

        return instruction;
    } else {
        fprintf(stderr, "Error: Label '%s' not found\n", label);
        exit(EXIT_FAILURE);
    }
}

uint32_t encodeImm26(InstrImm26 instr) {
    assert((instr.imm26 >> 26) == 0);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.imm26 & 0x3FFFFFF, 0);
}

uint32_t encodeNoOperand(InstrNoOperand instr) {
    return ENCODE(instr.opcode, 26);
}

uint32_t encodeInstruction(LabelMap *map, AsmInstr *inst_list, uint32_t currentPC) {
    Instruction *inst = inst_list->instruction;
    switch (inst_list->kind) {
        case INSTR_REGREG:
            return encodeRegReg(inst->regreg);
        case INSTR_REGIMM21:
            return encodeRegImm21(inst->regimm21);
        case INSTR_REG:
            return encodeReg(inst->reg);
        case INSTR_LABEL:
            return encodeLabel(map, inst->label, currentPC);
        case INSTR_IMM26:
            return encodeImm26(inst->imm26);
        case INSTR_REGLABEL: {
            uint32_t addr;
            if (!getLabelAddress(map, inst->reglabel.label, &addr)) {
                fprintf(stderr, "Error: Label '%s' not found\n", inst->reglabel.label);
                exit(EXIT_FAILURE);
            }
            if (addr >= (1u << 21)) {
                fprintf(stderr, "Error: Address 0x%X exceeds 21-bit immediate range for reg,label\n", addr);
                exit(EXIT_FAILURE);
            }
            InstrRegImm21 tmp = { .opcode = inst->reglabel.opcode, .reg1 = inst->reglabel.reg1, .imm21 = addr & 0x1FFFFF };
            return encodeRegImm21(tmp);
        }
        case INSTR_NO_OPERAND:
            return encodeNoOperand(inst->noOperand);
        // etc.
        default:
            fprintf(stderr, "Unknown opcode: %u\n", inst->base.opcode);
            exit(EXIT_FAILURE);
    }
}

MachineCode codeGen(AsmBlock *head) {
    LabelMap labelMap;

    initLabelMap(&labelMap);

    // First pass: assign addresses to labels
    uint32_t pc = 0;
    for (AsmBlock *line = head; line; line = line->next) {

        if (line->label && strlen(line->label) > 0) {
            mapLabelToAddress(&labelMap, line->label, pc);
            pc += (line->num_instrucitons) * sizeof(uint32_t);
            pc += (uint32_t)line->data_count; // account for data bytes
        }
    }

    uint32_t total_bytes = pc; // total output size in bytes
    uint8_t *machineCode = malloc(total_bytes);
    if (!machineCode) {
        perror("Failed to allocate machine code buffer");
        exit(EXIT_FAILURE);
    }

    // Second pass: encode instructions
    pc = 0;
    for (AsmBlock *line = head; line; line = line->next) {
        for (AsmInstr *inst = line->inst_list; inst; inst = inst->next) {
            uint32_t encoded;
            if (inst->needs_fixup) {
                // Assume it's a label-based instruction
                if (inst->kind == INSTR_LABEL) {
                    encoded = encodeLabel(&labelMap, inst->instruction->label, pc);
                } else if (inst->kind == INSTR_REGLABEL) {
                    encoded = encodeInstruction(&labelMap, inst, pc);
                } else {
                    encoded = encodeInstruction(&labelMap, inst, pc);
                }
            } else {
                encoded = encodeInstruction(&labelMap, inst, pc);
            }

            // Write 4 bytes in native endianness (consistent with prior uint32_t array write)
            memcpy(machineCode + pc, &encoded, sizeof(uint32_t));
            pc += 4;
        }

        // Append raw data bytes if present
        if (line->data_count > 0) {
            for (size_t i = 0; i < line->data_count; ++i) {
                machineCode[pc++] = encodeByte(line->data[i]);
            }
        }
    }

    MachineCode result = { .code = machineCode, .size = total_bytes };
    return result;
}
