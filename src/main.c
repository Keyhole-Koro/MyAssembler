#include <stdio.h>
#include <stdlib.h>

#include "assembler.h"

int main() {
    const char *file_path = "sample/sample1.masm";

    assembler(file_path);

    // optionally: print_tokens(head); or free_tokens(head);
    return 0;
}
