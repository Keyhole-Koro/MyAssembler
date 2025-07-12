#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>

Token *_lexer(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    Token *head = NULL;
    Token *cur = NULL;

    while (fgets(buffer, sizeof(buffer), file)) {
        cur = lexer(buffer, &head, cur);
    }

    fclose(file);

    return head;
}

MachineCode assembler(const char *file_path) {
    Token *tokens = _lexer(file_path);
    if (!tokens) {
        fprintf(stderr, "No tokens found.\n");
        return (MachineCode){NULL, 0};
    }

    for (Token *t = tokens; t != NULL; t = t->next) {
        printf("Token: Type=%s, Str='%s'\n", token_type_to_string(t->type), t->str);
    }

    LabelInstructionLine *parsed = parser(tokens);

    return codeGen(parsed);
}
