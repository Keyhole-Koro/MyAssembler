#include "unity/unity.h"
#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>   // mkdir / stat
#include <sys/types.h>
#include <sys/wait.h>   // waitpid
#include <unistd.h>     // fork
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

        MachineCode mc = assembler(path, NULL);

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
    const char *depfile = "tests/outputs/importFrom_main_dep_test.deps";

    MachineCode mc = assembler(source, NULL);
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

// Write `source` exactly as given -- a missing final newline included -- to
// tests/outputs/<name>.masm, and name the binary beside it.
// `source_path` and `binary_path` hold PATH_MAX bytes each.
static void write_source(const char *name, const char *source, char *source_path, char *binary_path) {
    ensure_output_dir();
    snprintf(source_path, PATH_MAX, "tests/outputs/%s.masm", name);
    snprintf(binary_path, PATH_MAX, "tests/outputs/%s.bin", name);
    FILE *file = fopen(source_path, "wb");
    TEST_ASSERT_NOT_NULL(file);
    fputs(source, file);
    fclose(file);
}

static MachineCode assemble_source(const char *name, const char *source) {
    char source_path[PATH_MAX];
    char binary_path[PATH_MAX];
    write_source(name, source, source_path, binary_path);
    return assembler(source_path, NULL);
}

// Assembling `source` must stop with an error saying `message` -- exit
// status 1, not a crash. The assembler exits on an error, so it runs in a
// child process, its errors going to tests/outputs/<name>.err.
static void assert_rejected(const char *name, const char *source, const char *message) {
    char source_path[PATH_MAX];
    char binary_path[PATH_MAX];
    char error_path[PATH_MAX];
    write_source(name, source, source_path, binary_path);
    snprintf(error_path, sizeof(error_path), "tests/outputs/%s.err", name);
    fflush(NULL);
    pid_t child = fork();
    TEST_ASSERT_TRUE(child >= 0);
    if (child == 0) {
        if (!freopen(error_path, "w", stderr)) _exit(2);
        assembler(source_path, NULL);
        _exit(0);
    }
    int status = 0;
    TEST_ASSERT_EQUAL_INT(child, waitpid(child, &status, 0));
    TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(status), "the assembler crashed");
    TEST_ASSERT_EQUAL_INT(1, WEXITSTATUS(status));
    FILE *file = fopen(error_path, "r");
    TEST_ASSERT_NOT_NULL(file);
    char errors[4096];
    size_t count = fread(errors, 1, sizeof(errors) - 1, file);
    errors[count] = '\0';
    fclose(file);
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(errors, message), errors);
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

// A shared label is private to its object, which refers to it by its private
// name; it is also in the table under its own name, as SYMBOL_SHARED, for
// another object to name (a test harness calling a test function).
void test_shared_label_is_private_and_named(void) {
    MachineCode mc = assemble_source("sharedLabel",
        ".shared helper\n__START__:\n  movi r1, helper\n  halt\nhelper:\n  halt\n");
    const ObjSymbol *shared = NULL;
    const ObjSymbol *private_name = NULL;
    for (size_t i = 0; i < mc.symbol_count; i++) {
        if (strcmp(mc.symbols[i].name, "helper") == 0) shared = &mc.symbols[i];
        else if (strncmp(mc.symbols[i].name, "helper" PRIVATE_LABEL_MARK, 8) == 0) private_name = &mc.symbols[i];
    }
    TEST_ASSERT_NOT_NULL(shared);
    TEST_ASSERT_NOT_NULL(private_name);
    TEST_ASSERT_EQUAL_UINT32(SYMBOL_SHARED, shared->type);
    TEST_ASSERT_EQUAL_UINT32(SYMBOL_DEFINED, private_name->type);
    TEST_ASSERT_EQUAL_UINT32(12, shared->offset);
    TEST_ASSERT_EQUAL_UINT32(12, private_name->offset);
    // Its own address load names it privately.
    TEST_ASSERT_EQUAL_size_t(2, mc.reloc_count);
    TEST_ASSERT_EQUAL_STRING(private_name->name, mc.relocs[0].symbol_name);
    free(mc.code);
}

// A label operand may be the file's last token too.
void test_operand_on_last_line_without_newline(void) {
    MachineCode jump = assemble_source("jumpAtEnd", "__START__:\n  jmp __START__");
    const uint8_t jump_code[] = {0x34, 0x00, 0x00, 0x00}; // jmp, offset 0
    TEST_ASSERT_EQUAL_size_t(sizeof(jump_code), jump.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(jump_code, jump.code, sizeof(jump_code));
    free(jump.code);

    // An address load is two instructions, each relocated with half the address.
    MachineCode load = assemble_source("loadAtEnd", "__START__:\n  movi r1, __START__");
    const uint8_t load_code[] = {
        0x94, 0x20, 0x00, 0x00, // lui r1, 0: bits 31..11
        0x64, 0x20, 0x00, 0x00, // addis r1, 0: bits 10..0
    };
    TEST_ASSERT_EQUAL_size_t(sizeof(load_code), load.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(load_code, load.code, sizeof(load_code));
    TEST_ASSERT_EQUAL_size_t(2, load.reloc_count);
    TEST_ASSERT_EQUAL_UINT32(RELOC_HI21, load.relocs[0].type);
    TEST_ASSERT_EQUAL_UINT32(0, load.relocs[0].offset);
    TEST_ASSERT_EQUAL_UINT32(RELOC_LO11, load.relocs[1].type);
    TEST_ASSERT_EQUAL_UINT32(4, load.relocs[1].offset);
    free(load.code);
}

// A file that ends straight after `mov r1,` is an error, not a crash.
void test_missing_operand_at_end_of_file_is_an_error(void) {
    assert_rejected("operandMissingAtEnd", "__START__:\n  mov r1,",
                    "Expected REGISTER or NUMBER after COMMA but found: <eof>");
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
// byte unless the object format does. After such a change, regenerate it
// with this one command:
//   ./build/myas tests/golden/objectLayout.masm tests/outputs/objectLayout.mbin
//       --obj tests/golden/objectLayout.mobj
void test_object_matches_golden(void) {
    ensure_output_dir();
    MachineCode mc = assembler("tests/golden/objectLayout.masm", NULL);
    TEST_ASSERT_TRUE(write_object("tests/outputs/objectLayout.mobj", &mc));

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

// An object that cannot be written is a failure the caller hears of: a
// directory that is not there, or a device that is full.
void test_write_object_reports_failure(void) {
    ensure_output_dir();
    MachineCode mc = assembler("tests/golden/objectLayout.masm", NULL);
    TEST_ASSERT_FALSE(write_object("tests/outputs/no-such-directory/objectLayout.mobj", &mc));
    if (access("/dev/full", W_OK) == 0) {
        TEST_ASSERT_FALSE(write_object("/dev/full", &mc));
    }
    free(mc.code);
}

// Two private labels too long for the object's names, alike in their first
// 70 bytes, keep names of their own there: each is cut short to leave room
// for the `-[module:n]` that makes it private and unique, and each address
// load is relocated against its own label.
void test_long_private_labels_keep_distinct_names(void) {
    char prefix[71];
    memset(prefix, 'a', 70);
    prefix[70] = '\0';
    char source[1024];
    snprintf(source, sizeof(source),
             "__START__:\n"
             "  movi r1, %s_one\n"
             "  movi r2, %s_two\n"
             "  halt\n"
             "%s_one:\n"
             "  halt\n"
             "%s_two:\n"
             "  halt\n",
             prefix, prefix, prefix, prefix);
    MachineCode mc = assemble_source("longPrivateLabels", source);

    const ObjSymbol *one = NULL;
    const ObjSymbol *two = NULL;
    for (size_t i = 0; i < mc.symbol_count; i++) {
        if (mc.symbols[i].offset == 20) one = &mc.symbols[i];
        if (mc.symbols[i].offset == 24) two = &mc.symbols[i];
    }
    TEST_ASSERT_NOT_NULL(one);
    TEST_ASSERT_NOT_NULL(two);
    TEST_ASSERT_TRUE(strcmp(one->name, two->name) != 0);
    TEST_ASSERT_NOT_NULL(strstr(one->name, PRIVATE_LABEL_MARK));
    TEST_ASSERT_NOT_NULL(strstr(two->name, PRIVATE_LABEL_MARK));
    TEST_ASSERT_TRUE(strlen(one->name) < sizeof(((struct SymbolEntry *)0)->name));
    TEST_ASSERT_TRUE(strlen(two->name) < sizeof(((struct SymbolEntry *)0)->name));

    TEST_ASSERT_EQUAL_size_t(4, mc.reloc_count);
    TEST_ASSERT_EQUAL_STRING(one->name, mc.relocs[0].symbol_name);
    TEST_ASSERT_EQUAL_STRING(one->name, mc.relocs[1].symbol_name);
    TEST_ASSERT_EQUAL_STRING(two->name, mc.relocs[2].symbol_name);
    TEST_ASSERT_EQUAL_STRING(two->name, mc.relocs[3].symbol_name);

    // The object holds the names whole.
    ensure_output_dir();
    TEST_ASSERT_TRUE(write_object("tests/outputs/longPrivateLabels.mobj", &mc));
    size_t size = 0;
    unsigned char *object = read_whole_file("tests/outputs/longPrivateLabels.mobj", &size);
    TEST_ASSERT_NOT_NULL(object);
    struct FileHeader header;
    TEST_ASSERT_TRUE(size >= sizeof(header));
    memcpy(&header, object, sizeof(header));
    size_t at = sizeof(header) + header.text_size + header.data_size + header.collect_size;
    TEST_ASSERT_EQUAL_UINT32(mc.symbol_count, header.symtable_count);
    TEST_ASSERT_TRUE(size >= at + header.symtable_count * sizeof(struct SymbolEntry));
    for (size_t i = 0; i < header.symtable_count; i++) {
        struct SymbolEntry entry;
        memcpy(&entry, object + at + i * sizeof(entry), sizeof(entry));
        TEST_ASSERT_EQUAL_STRING(mc.symbols[i].name, entry.name);
    }
    free(object);
    free(mc.code);
}

// An immediate the instruction has no room for is an error at its line,
// not another value, and not a crash.
void test_out_of_range_immediates_are_rejected(void) {
    assert_rejected("immediateTooWide", "__START__:\n  movi r1, 0x100000005\n",
                    ":2:12: error: Immediate 4294967301 is out of range");
    assert_rejected("immediateTooLarge", "__START__:\n  movi r1, 0x200000\n",
                    ":2:12: error: Immediate 2097152 is out of range");
    assert_rejected("immediateTooNegative", "__START__:\n  movi r1, -3000000\n",
                    ":2:12: error: Immediate -3000000 is out of range");
    assert_rejected("immediateJustTooNegative", "__START__:\n  movi r1, -1048577\n",
                    ":2:12: error: Immediate -1048577 is out of range");
    assert_rejected("immediateTooWideJump", "__START__:\n  jmp 99999999\n",
                    ":2:7: error: Immediate 99999999 is out of range");
}

// The widest immediates that fit encode as they always have.
void test_immediates_at_the_ends_of_their_range(void) {
    MachineCode mc = assemble_source("immediateEnds",
                                     "__START__:\n"
                                     "  movi r1, 0x1FFFFF\n"
                                     "  movi r1, -1048576\n"
                                     "  movi r1, 0\n"
                                     "  jmp 0x3FFFFFF\n");
    const uint8_t expected[] = {
        0x08, 0x3F, 0xFF, 0xFF, // movi r1, 0x1FFFFF
        0x08, 0x30, 0x00, 0x00, // movi r1, -1048576: 0x100000 in 21 bits
        0x08, 0x20, 0x00, 0x00, // movi r1, 0
        0x37, 0xFF, 0xFF, 0xFF, // jmp 0x3FFFFFF
    };
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), mc.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, mc.code, sizeof(expected));
    free(mc.code);
}

// `text` followed by `count` copies of `repeat`, then `tail`, in a new string.
static char *repeated(const char *text, const char *repeat, size_t count, const char *tail) {
    size_t length = strlen(text) + strlen(repeat) * count + strlen(tail);
    char *source = malloc(length + 1);
    TEST_ASSERT_NOT_NULL(source);
    char *at = stpcpy(source, text);
    for (size_t i = 0; i < count; i++) at = stpcpy(at, repeat);
    strcpy(at, tail);
    return source;
}

// A line is read whole, however long: a comment runs to the end of its
// line, and the next line is the line after.
void test_long_comment_line(void) {
    char *source = repeated("; ", "x", 9000, "\n__START__:\n  halt\n");
    MachineCode mc = assemble_source("longCommentLine", source);
    const uint8_t halt[] = {0xFC, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_size_t(sizeof(halt), mc.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(halt, mc.code, sizeof(halt));
    free(mc.code);
    free(source);
}

// A number at byte 8189..8191 of a `.byte` line -- where an 8192-byte line
// buffer used to split the line -- is one number.
void test_long_byte_line(void) {
    char *source = repeated("__START__:\n  halt\ndata:\n  .byte  ", "1,", 4090, "123,7\n");
    MachineCode mc = assemble_source("longByteLine", source);
    TEST_ASSERT_EQUAL_size_t(4 + 4092, mc.size);
    for (size_t i = 4; i < 4 + 4090; i++) TEST_ASSERT_EQUAL_HEX8(1, mc.code[i]);
    TEST_ASSERT_EQUAL_HEX8(123, mc.code[4 + 4090]);
    TEST_ASSERT_EQUAL_HEX8(7, mc.code[4 + 4091]);
    free(mc.code);
    free(source);
}

// An error after a long line is reported at its own line, and shows it.
void test_error_after_long_line_names_its_line(void) {
    char *source = repeated("; ", "x", 9000, "\n__START__:\n  movi r1, 0x200000\n");
    assert_rejected("errorAfterLongLine", source,
                    ":3:12: error: Immediate 2097152 is out of range (-1048576..2097151)\n"
                    "    movi r1, 0x200000\n");
    free(source);
}

// A byte outside ASCII (UTF-8 text) may sit in a comment or a string, and
// is an error anywhere else.
void test_non_ascii_bytes_only_in_comments_and_strings(void) {
    MachineCode mc = assemble_source("nonAsciiText",
                                     "__START__: ; caf\xC3\xA9\n"
                                     "  .func \"caf\xC3\xA9\" \"src/\xC3\xA9.mln\"\n"
                                     "  halt ; \xE2\x80\x94\n");
    const uint8_t halt[] = {0xFC, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_size_t(sizeof(halt), mc.size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(halt, mc.code, sizeof(halt));
    free(mc.code);

    assert_rejected("nonAsciiCode", "__START__:\n  halt \xC3\xA9\n",
                    "Unknown character at line 2 col 8");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_all_samples);
    RUN_TEST(test_dependency_file_uses_import_from_path);
    RUN_TEST(test_object_matches_golden);
    RUN_TEST(test_write_object_reports_failure);
    RUN_TEST(test_label_on_last_line_without_newline);
    RUN_TEST(test_operand_on_last_line_without_newline);
    RUN_TEST(test_shared_label_is_private_and_named);
    RUN_TEST(test_missing_operand_at_end_of_file_is_an_error);
    RUN_TEST(test_negative_number_ends_at_its_last_digit);
    RUN_TEST(test_negative_numbers_in_operands_and_lists);
    RUN_TEST(test_non_ascii_bytes_only_in_comments_and_strings);
    RUN_TEST(test_long_private_labels_keep_distinct_names);
    RUN_TEST(test_out_of_range_immediates_are_rejected);
    RUN_TEST(test_immediates_at_the_ends_of_their_range);
    RUN_TEST(test_long_comment_line);
    RUN_TEST(test_long_byte_line);
    RUN_TEST(test_error_after_long_line_names_its_line);
    return UNITY_END();
}
