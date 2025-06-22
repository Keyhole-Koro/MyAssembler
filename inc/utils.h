#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

const char* token_type_to_string(TokenType type);

void print_binary32(uint32_t value);

#endif