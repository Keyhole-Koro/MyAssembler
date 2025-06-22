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
