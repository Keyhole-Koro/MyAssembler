#include "unity/unity.h"
#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>   // mkdir / stat
#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <direct.h>   // _mkdir on Windows
  #define MKDIR(path) _mkdir(path)
#else
  #include <errno.h>
  #define MKDIR(path) mkdir(path, 0755)
#endif

void setUp(void) { /* Called before each test */ }
void tearDown(void) { /* Called after each test */ }

/* Ensure bin/ exists (no error if it already does) */
static void ensure_output_dir(void) {
    const char *output_dir = "tests/outputs";
    struct stat st;
    if (stat(output_dir, &st) == -1) {           // Not found ⇒ create it
        if (MKDIR(output_dir) != 0) {
            perror("Unable to create 'bin' directory");
            exit(EXIT_FAILURE);
        }
    } else if (!S_ISDIR(st.st_mode)) {        // Exists but not a dir
        fprintf(stderr, "'bin' exists but is not a directory\n");
        exit(EXIT_FAILURE);
    }
}

void test_all_samples(void) {
    const char *folder = "tests/inputs/";
    DIR *dir = opendir(folder);
    if (!dir) {
        perror("Failed to open sample folder");
        TEST_FAIL_MESSAGE("Could not open 'sample/' directory");
        return;
    }

    ensure_output_dir();     // <- make sure bin/ is ready

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);

        if (len > 5 && strcmp(name + len - 5, ".masm") == 0) {
            char asm_path[256];
            snprintf(asm_path, sizeof(asm_path), "%s%s", folder, name);
            printf("Testing file: %s\n", asm_path);
        
            // Remove the .masm extension from the file name
            char base_name[256];
            strncpy(base_name, name, len - 5); // Copy the name without the last 5 characters (.masm)
            base_name[len - 5] = '\0';        // Null-terminate the string
        
            MachineCode mc = assembler(asm_path);   // Assemble
        
            char bin_path[256];
            snprintf(bin_path, sizeof(bin_path), "tests/outputs/%s.bin", base_name);
            FILE *out = fopen(bin_path, "wb");
            if (!out) {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }
        
            fwrite(mc.code, sizeof(uint32_t), mc.size, out);
            fclose(out);
            free(mc.code);
        }
    }

    closedir(dir);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_samples);
    return UNITY_END();
}
