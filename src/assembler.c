#include <stdio.h>



void assember(char *file_path) {
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        perror("Error opening file");
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Process each line of the assembly file
        // For now, just print it to stdout
        printf("%s", line);
    }
    fclose(f);
    printf("Assembly completed for file: %s\n", file_path);
}