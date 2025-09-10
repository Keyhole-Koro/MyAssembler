#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "instructions.h"

static void to_lower_copy(const char *src, char *dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) return;
    size_t i = 0;
    for (; src[i] && i + 1 < dst_size; ++i) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

static uint8_t get_opcode_ci(const char* mnemonic) {
    char lower[64];
    to_lower_copy(mnemonic, lower, sizeof(lower));
    return opcode_from_mnemonic(lower);
}

uint32_t encode_instruction(const char* mnemonic, uint8_t reg1, uint8_t reg2, uint16_t imm16) {
    uint8_t opcode = get_opcode_ci(mnemonic);
    uint32_t inst = 0;
    inst |= ((uint32_t)opcode & 0x3F) << 26;
    inst |= ((uint32_t)reg1   & 0x1F) << 21;
    inst |= ((uint32_t)reg2   & 0x1F) << 16;
    inst |= imm16;
    return inst;
}
