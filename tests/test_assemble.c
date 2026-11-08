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

void test_dependency_file_uses_import_from_path(void) {
    ensure_output_dir();
    const char *source = "tests/succeed/function/importFrom_main.masm";
    const char *binary = "tests/outputs/importFrom_main_dep_test.bin";
    const char *depfile = "tests/outputs/importFrom_main_dep_test.deps";

    MachineCode mc = assembler(source, binary);
    TEST_ASSERT_TRUE(write_dependency_file(depfile, source));

    char helper[PATH_MAX];
    TEST_ASSERT_NOT_NULL(realpath(
        "tests/succeed/function/importFrom_helper.masm", helper));

    FILE *file = fopen(depfile, "rb");
    TEST_ASSERT_NOT_NULL(file);
    char contents[PATH_MAX + 64];
    size_t count = fread(contents, 1, sizeof(contents) - 1, file);
    contents[count] = '\0';
    fclose(file);

    TEST_ASSERT_TRUE(strncmp(contents, "MYDEPS 1\n", 9) == 0);
    TEST_ASSERT_NOT_NULL(strstr(contents, "masm\t"));
    TEST_ASSERT_NOT_NULL(strstr(contents, helper));

    free(mc.code);
}

// Assemble `source`, written exactly as given -- a missing final newline
// included -- to tests/outputs/<name>.masm.
static MachineCode assemble_source(const char *name, const char *source) {
    ensure_output_dir();
    char source_path[PATH_MAX];
    char binary_path[PATH_MAX];
    snprintf(source_path, sizeof(source_path), "tests/outputs/%s.masm", name);
    snprintf(binary_path, sizeof(binary_path), "tests/outputs/%s.bin", name);
    FILE *file = fopen(source_path, "wb");
    TEST_ASSERT_NOT_NULL(file);
    fputs(source, file);
    fclose(file);
    return assembler(source_path, binary_path);
}

// The symbol named `label`, under its own name or the private name it gets,
// `label-[module:n]`.
static const ObjSymbol *find_symbol(const MachineCode *mc, const char *label) {
    size_t length = strlen(label);
    for (size_t i = 0; i < mc->symbol_count; i++) {
        const char *name = mc->symbols[i].name;
        if (strncmp(name, label, length) == 0 &&
            (name[length] == '\0' ||
             strncmp(name + length, PRIVATE_LABEL_MARK, sizeof(PRIVATE_LABEL_MARK) - 1) == 0)) {
            return &mc->symbols[i];
        }
    }
    return NULL;
}

// A file may end without a newline. Its last line, `end:`, labels the end
// of the text, as it does when a newline follows.
void test_label_on_last_line_without_newline(void) {
    MachineCode mc = assemble_source("labelAtEnd", "__START__:\n  halt\nend:");
    TEST_ASSERT_EQUAL_size_t(4, mc.size);
    const ObjSymbol *end = find_symbol(&mc, "end");
    TEST_ASSERT_NOT_NULL(end);
    TEST_ASSERT_EQUAL_UINT32(SECTION_TEXT, end->section);
    TEST_ASSERT_EQUAL_UINT32(4, end->offset);
    free(mc.code);
}

typedef struct {
    TokenType type;
    const char *text;
} ExpectedToken;

// `line`'s tokens must be `expected`, and no more.
static void assert_tokens(const char *line, const ExpectedToken *expected, size_t count) {
    Token *head = NULL;
    lexer(line, &head, NULL, 1);
    Token *token = head->next; // the head is an empty token
    for (size_t i = 0; i < count; i++, token = token->next) {
        TEST_ASSERT_NOT_NULL(token);
        TEST_ASSERT_EQUAL_INT(expected[i].type, token->type);
        TEST_ASSERT_EQUAL_STRING(expected[i].text, token->str);
    }
    TEST_ASSERT_NULL(token);
}

#define ASSERT_TOKENS(line, ...)                                               \
    do {                                                                       \
        const ExpectedToken expected[] = {__VA_ARGS__};                        \
        assert_tokens(line, expected, sizeof(expected) / sizeof(expected[0])); \
    } while (0)

// A negative number ends at its last digit: whatever follows is lexed as it
// would be after any other token, and the end of the line is the end.
void test_negative_number_ends_at_its_last_digit(void) {
    ASSERT_TOKENS("-1,x\n", {NEGATIVE_NUMBER, "-1"}, {COMMA, ","}, {LABEL, "x"}, {NEWLINE, "\n"});
    ASSERT_TOKENS("-1;comment\n", {NEGATIVE_NUMBER, "-1"}, {NEWLINE, "\n"});
    // The token is the value, shorter than the text it was read from.
    ASSERT_TOKENS("-007,8", {NEGATIVE_NUMBER, "-7"}, {COMMA, ","}, {NUMBER, "8"});
    // Nothing after the line's terminator is read.
    static const char last_line[] = "-1\0, 2";
    ASSERT_TOKENS(last_line, {NEGATIVE_NUMBER, "-1"});
}

// A negative number as an immediate and in `.word` and `.byte` lists, each
// followed straight away by a comment or a comma.
void test_negative_numbers_in_operands_and_lists(void) {
    MachineCode mc = assemble_source("negativeNumbers",
                                     "__START__:\n"
                                     "  movi r1, -1;a comment\n"
                                     "  halt\n"
                                     "words:\n"
                                     "  .word -1, words, -010,3\n"
                                     "bytes:\n"
                                     "  .byte -1,-2, 3\n");
    const uint8_t expected[] = {
        0x08, 0x3F, 0xFF, 0xFF, // movi r1, -1: the immediate is 21 bits
        0xFC, 0x00, 0x00, 0x00, // halt
        0xFF, 0xFF, 0xFF, 0xFF, // -1
        0x00, 0x00, 0x00, 0x00, // words, for the linker to fill in
        0xFF, 0xFF, 0xFF, 0xF6, // -010 is -10: the digits after a sign are decimal
        0x00, 0x00, 0x00, 0x03,
        0xFF, 0xFE, 0x03, 0x00, // -1, -2, 3, then padding to a word
    };
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), mc.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, mc.code, sizeof(expected));
    TEST_ASSERT_EQUAL_size_t(1, mc.reloc_count);
    TEST_ASSERT_EQUAL_UINT32(RELOC_WORD32, mc.relocs[0].type);
    TEST_ASSERT_EQUAL_UINT32(12, mc.relocs[0].offset);
    free(mc.code);
}

static unsigned char *read_whole_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    unsigned char *bytes = malloc(length > 0 ? (size_t)length : 1);
    *size = bytes ? fread(bytes, 1, (size_t)length, file) : 0;
    fclose(file);
    return bytes;
}

// The object written for tests/golden/objectLayout.masm must not change by a
// byte unless the object format does. After such a change, regenerate it:
//   ./build/myas tests/golden/objectLayout.masm tests/outputs/objectLayout.mbin \
//       --obj tests/golden/objectLayout.mobj
void test_object_matches_golden(void) {
    ensure_output_dir();
    MachineCode mc = assembler("tests/golden/objectLayout.masm", "tests/outputs/objectLayout.mbin");
    write_object("tests/outputs/objectLayout.mobj", &mc);

    size_t expected_size = 0;
    size_t actual_size = 0;
    unsigned char *expected = read_whole_file("tests/golden/objectLayout.mobj", &expected_size);
    unsigned char *actual = read_whole_file("tests/outputs/objectLayout.mobj", &actual_size);
    TEST_ASSERT_NOT_NULL(expected);
    TEST_ASSERT_NOT_NULL(actual);
    TEST_ASSERT_EQUAL_size_t(expected_size, actual_size);
    TEST_ASSERT_EQUAL_MEMORY(expected, actual, expected_size);

    free(expected);
    free(actual);
    free(mc.code);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_samples);
    RUN_TEST(test_dependency_file_uses_import_from_path);
    RUN_TEST(test_object_matches_golden);
    RUN_TEST(test_label_on_last_line_without_newline);
    RUN_TEST(test_negative_number_ends_at_its_last_digit);
    RUN_TEST(test_negative_numbers_in_operands_and_lists);
    return UNITY_END();
}
