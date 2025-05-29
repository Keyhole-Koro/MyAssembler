#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

int main() {
    const char *file_path = "sample/sample1.masm";
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    char buffer[256];
    Token *head = NULL;
    Token *cur = NULL;

    while (fgets(buffer, sizeof(buffer), file)) {
        cur = lexer(buffer, &head, cur);
    }

    for (Token *token = head; token != NULL; token = token->next) {
        printf("Token: Type=%d, Str=%s\n", token->type, token->str);
    }

    fclose(file);
    // optionally: print_tokens(head); or free_tokens(head);
    return 0;
}
