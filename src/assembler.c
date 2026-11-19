#include "assembler.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ObjectFormat.h"

Token *_lexer(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Generated `.byte` lines for string literals run ~6 chars per byte, so a
    // long UI string makes a long line. Each line is read whole, however
    // long: a line read in pieces would be split mid-token.
    char *buffer = NULL;
    size_t capacity = 0;
    Token *head = NULL;
    Token *cur = NULL;

    int line = 1;
    while (getline(&buffer, &capacity, file) != -1) {
        cur = lexer(buffer, &head, cur, line);
        line++;
    }

    free(buffer);
    fclose(file);

    return head;
}

MachineCode assembler(const char *file_path, const char *report_path) {
    parser_set_source_file(file_path);
    Token *tokens = _lexer(file_path);
    if (!tokens) {
        fprintf(stderr, "No tokens found.\n");
        return (MachineCode){0};
    }

    AsmBlock *parsed = parser(tokens);

    // Without a report path nothing is written: the report runs to several
    // times the size of the source, and a system build assembles hundreds of
    // files.
    write_debug_report(report_path, tokens, parsed);

    size_t import_count = 0;
    const char **imports = parser_get_imports(&import_count);
    size_t export_count = 0;
    const char **exports = parser_get_exports(&export_count);
    size_t share_count = 0;
    const char **shares = parser_get_shares(&share_count);
    return codeGen(parsed, imports, import_count, exports, export_count, shares, share_count, file_path);
}

typedef struct SourceDependency {
    char *canonical_path;
    const char *kind;
} SourceDependency;

static int path_has_suffix(const char *path, const char *suffix) {
    size_t path_len = strlen(path);
    size_t suffix_len = strlen(suffix);
    return path_len >= suffix_len &&
           strcmp(path + path_len - suffix_len, suffix) == 0;
}

static void free_dependencies(SourceDependency *dependencies, size_t count) {
    for (size_t i = 0; i < count; i++) free(dependencies[i].canonical_path);
    free(dependencies);
}

int write_dependency_file(const char *depfile_path, const char *file_path) {
    if (!depfile_path || !file_path) return 0;

    char source_path[PATH_MAX];
    if (!realpath(file_path, source_path)) {
        fprintf(stderr, "Failed to resolve assembler source path: %s\n", file_path);
        return 0;
    }
    char *slash = strrchr(source_path, '/');
    if (!slash) {
        fprintf(stderr, "Assembler source path has no directory: %s\n", source_path);
        return 0;
    }
    *slash = '\0';

    size_t import_path_count = 0;
    const char **import_paths = parser_get_import_paths(&import_path_count);
    SourceDependency *dependencies = NULL;
    size_t dependency_count = 0;
    for (size_t i = 0; i < import_path_count; i++) {
        char unresolved[PATH_MAX];
        int written;
        if (import_paths[i][0] == '/') {
            written = snprintf(unresolved, sizeof(unresolved), "%s", import_paths[i]);
        } else {
            written = snprintf(unresolved, sizeof(unresolved), "%s/%s",
                               source_path, import_paths[i]);
        }
        if (written < 0 || (size_t)written >= sizeof(unresolved)) {
            fprintf(stderr, "Assembler dependency path is too long: %s\n", import_paths[i]);
            free_dependencies(dependencies, dependency_count);
            return 0;
        }

        char canonical[PATH_MAX];
        if (!realpath(unresolved, canonical)) {
            fprintf(stderr, "Failed to resolve assembler dependency '%s' from '%s'\n",
                    import_paths[i], file_path);
            free_dependencies(dependencies, dependency_count);
            return 0;
        }
        const char *kind = NULL;
        if (path_has_suffix(canonical, ".mln")) kind = "mln";
        else if (path_has_suffix(canonical, ".masm")) kind = "masm";
        else {
            fprintf(stderr, "Unsupported assembler dependency type: %s\n", canonical);
            free_dependencies(dependencies, dependency_count);
            return 0;
        }
        if (strchr(canonical, '\n') || strchr(canonical, '\r') || strchr(canonical, '\t')) {
            fprintf(stderr, "Assembler dependency path contains control characters: %s\n",
                    canonical);
            free_dependencies(dependencies, dependency_count);
            return 0;
        }

        int duplicate = 0;
        for (size_t j = 0; j < dependency_count; j++) {
            if (strcmp(dependencies[j].canonical_path, canonical) == 0) {
                duplicate = 1;
                break;
            }
        }
        if (duplicate) continue;

        SourceDependency *grown = realloc(
            dependencies, sizeof(SourceDependency) * (dependency_count + 1));
        if (!grown) {
            fprintf(stderr, "Out of memory while collecting assembler dependencies\n");
            free_dependencies(dependencies, dependency_count);
            return 0;
        }
        dependencies = grown;
        dependencies[dependency_count].canonical_path = strdup(canonical);
        dependencies[dependency_count].kind = kind;
        if (!dependencies[dependency_count].canonical_path) {
            fprintf(stderr, "Out of memory while collecting assembler dependencies\n");
            free_dependencies(dependencies, dependency_count);
            return 0;
        }
        dependency_count++;
    }

    FILE *file = fopen(depfile_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open dependency file: %s\n", depfile_path);
        free_dependencies(dependencies, dependency_count);
        return 0;
    }
    fprintf(file, "MYDEPS 1\n");
    for (size_t i = 0; i < dependency_count; i++) {
        fprintf(file, "%s\t%s\n", dependencies[i].kind,
                dependencies[i].canonical_path);
    }
    int ok = fclose(file) == 0;
    if (!ok) fprintf(stderr, "Failed to write dependency file: %s\n", depfile_path);
    free_dependencies(dependencies, dependency_count);
    return ok;
}

// Write `size` bytes; 1 if all were written.
static int write_bytes(FILE *f, const void *bytes, size_t size) {
    return size == 0 || fwrite(bytes, 1, size, f) == size;
}

int write_object(const char *obj_path, const MachineCode *mc) {
    if (!obj_path || !mc) return 0;
    FILE *f = fopen(obj_path, "wb");
    if (!f) {
        perror("Failed to open .obj file");
        return 0;
    }
    int ok = 1;

    struct FileHeader hdr = {0};
    hdr.magic = LINKER_MAGIC;
    hdr.text_size = (uint32_t)mc->size;
    hdr.data_size = (uint32_t)mc->data_size;
    hdr.symtable_count = (uint32_t)mc->symbol_count;
    hdr.reloc_count = (uint32_t)mc->reloc_count;
    hdr.collect_count = (uint32_t)mc->collect_count;
    hdr.collect_size = (uint32_t)mc->blob_size;
    ok &= write_bytes(f, &hdr, sizeof(hdr));

    // text section
    ok &= write_bytes(f, mc->code, mc->size);

    // data section
    ok &= write_bytes(f, mc->data, mc->data_size);

    // collected-section blob
    ok &= write_bytes(f, mc->blob, mc->blob_size);

    // symbols
    for (size_t i = 0; i < mc->symbol_count; ++i) {
        struct SymbolEntry se = {0};
        const ObjSymbol *src = &mc->symbols[i];
        strncpy(se.name, src->name ? src->name : "", sizeof(se.name) - 1);
        se.type = src->type;
        se.section = src->section;
        se.offset = src->offset;
        ok &= write_bytes(f, &se, sizeof(se));
    }

    // relocs
    for (size_t i = 0; i < mc->reloc_count; ++i) {
        struct RelocEntry re = {0};
        const ObjReloc *src = &mc->relocs[i];
        re.offset = src->offset;
        strncpy(re.symbol_name, src->symbol_name ? src->symbol_name : "", sizeof(re.symbol_name) - 1);
        re.type = src->type;
        re.section = src->section;
        ok &= write_bytes(f, &re, sizeof(re));
    }

    // collected-section chunks
    for (size_t i = 0; i < mc->collect_count; ++i) {
        struct CollectEntry ce = {0};
        const ObjCollect *src = &mc->collects[i];
        strncpy(ce.name, src->name ? src->name : "", sizeof(ce.name) - 1);
        ce.offset = src->offset;
        ce.size = src->size;
        ok &= write_bytes(f, &ce, sizeof(ce));
    }

    // debug trailer, for sources that carry `.func`/`.loc`/`.cost`
    if (mc->debug.row_count > 0) {
        struct DebugHeader dh = {0};
        dh.magic = DEBUG_MAGIC;
        dh.string_size = (uint32_t)mc->debug.string_size;
        dh.func_count = (uint32_t)mc->debug.func_count;
        dh.row_count = (uint32_t)mc->debug.row_count;
        ok &= write_bytes(f, &dh, sizeof(dh));
        ok &= write_bytes(f, mc->debug.strings, mc->debug.string_size);
        for (size_t i = 0; i < mc->debug.func_count; ++i) {
            const ObjDebugFunc *src = &mc->debug.funcs[i];
            struct DebugFunc df = {src->offset, src->name, src->file};
            ok &= write_bytes(f, &df, sizeof(df));
        }
        for (size_t i = 0; i < mc->debug.row_count; ++i) {
            const ObjDebugRow *src = &mc->debug.rows[i];
            struct DebugRow dr = {src->offset, src->file, src->line, src->cost};
            ok &= write_bytes(f, &dr, sizeof(dr));
        }
    }

    if (fclose(f) != 0) ok = 0;
    if (!ok) fprintf(stderr, "Failed to write .obj file: %s\n", obj_path);
    return ok;
}
