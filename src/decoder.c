#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    const char* name;
    uint8_t opcode;
} OpcodeEntry;

OpcodeEntry opcode_table[] = {
    {"MOV",  0x00},
    {"MOVI", 0x01},
    {"LD",   0x02},
    {"ST",   0x03},
    {"ADD",  0x04},
    {"SUB",  0x05},
    {"CMP",  0x06},
    {"AND",  0x07},
    {"OR",   0x08},
    {"XOR",  0x09},
    {"SHL",  0x0A},
    {"SHR",  0x0B},
    {"JMP",  0x0C},
    {"JZ",   0x0D},
    {"JNZ",  0x0E},
    {"CALL", 0x0F},
    {"RET",  0x10}
};

uint8_t get_opcode(const char* mnemonic) {
    for (long unsigned int i = 0; i < sizeof(opcode_table) / sizeof(opcode_table[0]); i++) {
        if (strcmp(opcode_table[i].name, mnemonic) == 0) {
            return opcode_table[i].opcode;
    }
    }
    return 0xFF;
}

uint32_t encode_instruction(const char* mnemonic, uint8_t reg1, uint8_t reg2, uint16_t imm16) {
    uint8_t opcode = get_opcode(mnemonic);
    if (opcode == 0xFF) {
        printf("Invalid instruction: %s\n", mnemonic);
        return 0;
    }

    uint32_t inst = 0;
    inst |= ((uint32_t)opcode & 0x3F) << 26;
    inst |= ((uint32_t)reg1   & 0x1F) << 21;
    inst |= ((uint32_t)reg2   & 0x1F) << 16;
    inst |= imm16;

    return inst;
}