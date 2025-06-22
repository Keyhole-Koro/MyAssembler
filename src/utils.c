#include "utils.h"

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case COLON:       return "COLON";
        case COMMA:       return "COMMA";
        case PERIOD:      return "PERIOD";
        case IDENTIFIER:  return "IDENTIFIER";
        case INSTRUCTION: return "INSTRUCTION";
        case REGISTER:    return "REGISTER";
        case NUMBER:      return "NUMBER";
        case LABEL:       return "LABEL";
        case NEWLINE:     return "NEWLINE";
        default:          return "UNKNOWN";
    }
}


void print_binary32(uint32_t value) {
    for (int i = 31; i >= 0; --i) {
        printf("%c", (value >> i) & 1 ? '1' : '0');
        if (i % 4 == 0 && i != 0) printf(" "); // 4ビット区切り
    }
}
