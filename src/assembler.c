#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>

Token *_lexer(char *file_path) {
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

void assembler(char *file_path) {
    Token *tokens = _lexer(file_path);
    if (tokens == NULL) {
        fprintf(stderr, "No tokens found.\n");
        return;
    }

    for (Token *t = tokens; t != NULL; t = t->next) {
        printf("Token: Type=%d, Str=%s\n", t->type, t->str);
    }
}