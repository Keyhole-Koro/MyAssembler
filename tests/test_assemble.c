#include "unity/unity.h"
#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>   // mkdir / stat
#include <sys/types.h>
#include <limits.h>

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

static void sanitize_relpath(const char *rel_path, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; rel_path[i] != '\0' && j + 1 < out_size; ++i) {
        char ch = rel_path[i];
        if (ch == '/' || ch == '\\' || ch == '.') ch = '_';
        out[j++] = ch;
    }
    out[j] = '\0';
}

static void assemble_tree(const char *root, const char *current) {
    DIR *dir = opendir(current);
    if (!dir) {
        perror("Failed to open test folder");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", current, name);

        struct stat st;
        if (stat(path, &st) != 0) {
            perror("stat failed");
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(st.st_mode)) {
            assemble_tree(root, path);
            continue;
        }

        size_t len = strlen(name);
        if (len <= 5 || strcmp(name + len - 5, ".masm") != 0) continue;

        const char *rel = path + strlen(root);
        if (*rel == '/' || *rel == '\\') rel++;

        printf("Testing file: %s\n", path);

        char stem[PATH_MAX];
        sanitize_relpath(rel, stem, sizeof(stem));
        char *ext = strstr(stem, "_masm");
        if (ext && strcmp(ext, "_masm") == 0) *ext = '\0';

        size_t bin_path_len = strlen("tests/outputs/") + strlen(stem) + strlen(".bin") + 1;
        char *bin_path = malloc(bin_path_len);
        if (!bin_path) {
            perror("Failed to allocate output path");
            closedir(dir);
            exit(EXIT_FAILURE);
        }
        snprintf(bin_path, bin_path_len, "tests/outputs/%s.bin", stem);

        MachineCode mc = assembler(path, bin_path);

        FILE *out = fopen(bin_path, "wb");
        if (!out) {
            perror("Failed to open output file");
            free(bin_path);
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        fwrite(mc.code, 1, mc.size, out);
        fclose(out);
        free(bin_path);
        free(mc.code);
    }

    closedir(dir);
}

void test_all_samples(void) {
    const char *folder = "tests/succeed";

    ensure_output_dir();
    assemble_tree(folder, folder);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_samples);
    return UNITY_END();
}
