#include "codeGen.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#define ENCODE(op, shift) ((uint32_t)(op) << (shift))

// A single label/address pair
typedef struct {
    char *label;
    uint32_t address;
} LabelAddressMapping;

typedef struct {
    ObjSymbol *items;
    size_t count;
    size_t cap;
} SymbolVec;

typedef struct {
    ObjReloc *items;
    size_t count;
    size_t cap;
} RelocVec;

static int symbolvec_index(const SymbolVec *v, const char *name) {
    if (!v || !name) return -1;
    for (size_t i = 0; i < v->count; i++) {
        if (strcmp(v->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static bool is_imported(const char *name, const char **imports, size_t import_count) {
    if (!name) return false;
    for (size_t i = 0; i < import_count; i++) {
        if (strcmp(imports[i], name) == 0) return true;
    }
    return false;
}

static bool is_exported(const char *name, const char **exports, size_t export_count) {
    if (!name) return false;
    for (size_t i = 0; i < export_count; i++) {
        if (strcmp(exports[i], name) == 0) return true;
    }
    return false;
}

// A dynamic list of mappings
typedef struct {
    LabelAddressMapping *entries;
    size_t count;
    size_t capacity;
} LabelMap;

typedef struct {
    char *source_label;
    char *symbol_name;
} LabelSymbolMapping;

typedef struct {
    LabelSymbolMapping *entries;
    size_t count;
    size_t capacity;
} LabelSymbolMap;

void initLabelMap(LabelMap *map) {
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

static void initLabelSymbolMap(LabelSymbolMap *map) {
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

void mapLabelToAddress(LabelMap *map, const char *label, uint32_t address) {
    // Resize if needed
    if (map->count == map->capacity) {
        size_t new_capacity = map->capacity == 0 ? 8 : map->capacity * 2;
        map->entries = realloc(map->entries, new_capacity * sizeof(LabelAddressMapping));
        if (!map->entries) {
            perror("Failed to allocate memory for label map");
            exit(EXIT_FAILURE);
        }
        map->capacity = new_capacity;
    }

    // Add new entry
    map->entries[map->count].label = strdup(label);
    map->entries[map->count].address = address;
    map->count++;
}

static bool is_public_label(const char *label) {
    if (!label) return false;
    return strncmp(label, "f_", 2) == 0 || strcmp(label, "__START__") == 0;
}

static void labelsymbolmap_put(LabelSymbolMap *map, const char *source_label, const char *symbol_name) {
    if (map->count == map->capacity) {
        size_t new_capacity = map->capacity == 0 ? 8 : map->capacity * 2;
        map->entries = realloc(map->entries, new_capacity * sizeof(LabelSymbolMapping));
        if (!map->entries) {
            perror("Failed to allocate memory for label symbol map");
            exit(EXIT_FAILURE);
        }
        map->capacity = new_capacity;
    }

    map->entries[map->count].source_label = strdup(source_label);
    map->entries[map->count].symbol_name = strdup(symbol_name);
    map->count++;
}

// slow linear search
static const char *labelsymbolmap_get(const LabelSymbolMap *map, const char *source_label) {
    if (!map || !source_label) return NULL;
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->entries[i].source_label, source_label) == 0) {
            return map->entries[i].symbol_name;
        }
    }
    return NULL;
}

static void symbolvec_push(SymbolVec *v, const char *name, uint32_t type, uint32_t section, uint32_t offset) {
    if (v->count == v->cap) {
        size_t new_cap = v->cap == 0 ? 8 : v->cap * 2;
        v->items = realloc(v->items, new_cap * sizeof(ObjSymbol));
        if (!v->items) {
            perror("Failed to allocate symbol list");
            exit(EXIT_FAILURE);
        }
        v->cap = new_cap;
    }
    v->items[v->count].name = strdup(name ? name : "");
    v->items[v->count].type = type;
    v->items[v->count].section = section;
    v->items[v->count].offset = offset;
    v->count++;
}

static void relocvec_push(RelocVec *v, uint32_t offset, const char *sym, uint32_t type, uint32_t section) {
    if (v->count == v->cap) {
        size_t new_cap = v->cap == 0 ? 8 : v->cap * 2;
        v->items = realloc(v->items, new_cap * sizeof(ObjReloc));
        if (!v->items) {
            perror("Failed to allocate relocation list");
            exit(EXIT_FAILURE);
        }
        v->cap = new_cap;
    }
    v->items[v->count].offset = offset;
    v->items[v->count].symbol_name = strdup(sym ? sym : "");
    v->items[v->count].type = type;
    v->items[v->count].section = section;
    v->count++;
}

// Optional: Lookup function
int getLabelAddress(LabelMap *map, const char *label, uint32_t *out_address) {
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->entries[i].label, label) == 0) {
            *out_address = map->entries[i].address;
            return 1; // Found
        }
    }
    return 0; // Not found
}

uint8_t encodeByte(uint32_t value) {
    return (uint8_t)(value & 0xFFu);
}

uint32_t encodeRegReg(InstrRegReg instr) {
    assert(instr.reg1 < 32 && instr.reg2 < 32);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21) | ENCODE(instr.reg2, 16);
}

/**
 * Encodes an opcode and three five-bit register operands.
 * @param instr Parsed three-register instruction.
 * @return 32-bit machine instruction with operands at bits 25:11.
 */
uint32_t encodeRegRegReg(InstrRegRegReg instr) {
    assert(instr.reg1 < 32 && instr.reg2 < 32 && instr.reg3 < 32);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21) |
           ENCODE(instr.reg2, 16) | ENCODE(instr.reg3, 11);
}

uint32_t encodeRegImm21(InstrRegImm21 instr) {
    // An out-of-range immediate is a real input error, not an internal
    // inconsistency: it means the caller emitted something like
    // `movi r1, 0xDEADBEEF`, which this encoding has no room for. Report it
    // as such rather than aborting on an assert with no context, which gives
    // the caller a SIGABRT and no idea which instruction was at fault.
    if (instr.reg1 >= 32) {
        fprintf(stderr,
                "error: register index %u is out of range (max 31)\n",
                (unsigned)instr.reg1);
        exit(1);
    }
    if ((instr.imm21 >> 21) != 0) {
        fprintf(stderr,
                "error: immediate 0x%X does not fit in the 21 bits this "
                "instruction encodes (max 0x1FFFFF)\n"
                "  a wider constant has to be loaded from memory instead\n",
                instr.imm21);
        exit(1);
    }
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21) | ENCODE(instr.imm21 & 0x1FFFFF, 0);
}

uint32_t encodeReg(InstrReg instr) {
    assert(instr.reg1 < 32);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.reg1, 21);
}

uint32_t encodeLabel(LabelMap *labelMap, InstrLabel instr, uint32_t currentPC) {
    char *label = instr.label;
    uint32_t targetAddr = 0;

    if (getLabelAddress(labelMap, label, &targetAddr)) {

        int32_t offset = substraction_26bit(targetAddr, currentPC);

        if (offset < -(1 << 25) || offset >= (1 << 25)) {
            fprintf(stderr,
                "Error: Jump offset to label '%s' (%d bytes, aligned = %d) exceeds 26-bit signed range\n",
                label, offset >> 2, offset);
            exit(EXIT_FAILURE);
        }

        uint32_t offsetMasked = ((uint32_t)offset) & 0x03FFFFFF;

        uint32_t instruction = (instr.opcode << 26) | offsetMasked;

        return instruction;
    } else {
        fprintf(stderr, "Error: Label '%s' not found\n", label);
        exit(EXIT_FAILURE);
    }
}

uint32_t encodeImm26(InstrImm26 instr) {
    assert((instr.imm26 >> 26) == 0);
    return ENCODE(instr.opcode, 26) | ENCODE(instr.imm26 & 0x3FFFFFF, 0);
}

uint32_t encodeNoOperand(InstrNoOperand instr) {
    return ENCODE(instr.opcode, 26);
}

uint32_t encodeInstruction(LabelMap *map, AsmInstr *inst_list, uint32_t currentPC) {
    Instruction *inst = inst_list->instruction;
    switch (inst_list->kind) {
        case INSTR_REGREG:
            return encodeRegReg(inst->regreg);
        case INSTR_REGREGREG:
            return encodeRegRegReg(inst->regregreg);
        case INSTR_REGIMM21:
            return encodeRegImm21(inst->regimm21);
        case INSTR_REG:
            return encodeReg(inst->reg);
        case INSTR_LABEL:
            return encodeLabel(map, inst->label, currentPC);
        case INSTR_IMM26:
            return encodeImm26(inst->imm26);
        case INSTR_REGLABEL: {
            uint32_t addr;
            if (!getLabelAddress(map, inst->reglabel.label, &addr)) {
                // unresolved; caller will handle (placeholder)
                return ENCODE(inst->reglabel.opcode, 26) | ENCODE(inst->reglabel.reg1, 21);
            }
            if (addr >= (1u << 21)) {
                fprintf(stderr, "Error: Address 0x%X exceeds 21-bit immediate range for reg,label\n", addr);
                exit(EXIT_FAILURE);
            }
            InstrRegImm21 tmp = { .opcode = inst->reglabel.opcode, .reg1 = inst->reglabel.reg1, .imm21 = addr & 0x1FFFFF };
            return encodeRegImm21(tmp);
        }
        case INSTR_NO_OPERAND:
            return encodeNoOperand(inst->noOperand);
        // etc.
        default:
            fprintf(stderr, "Unknown opcode: %u\n", inst->base.opcode);
            exit(EXIT_FAILURE);
    }
}

static void build_module_tag(const char *module_tag, char *out, size_t out_size) {
    const char *base = path_basename(module_tag ? module_tag : "obj");

    size_t len = 0;
    while (base[len] && base[len] != '.' && len + 1 < out_size) {
        unsigned char ch = (unsigned char)base[len];
        out[len] = (isalnum(ch) || ch == '_') ? (char)ch : '_';
        len++;
    }
    out[len] = '\0';
    if (len == 0) snprintf(out, out_size, "obj");
}

// Offset of `text` in the pool, adding it if new.
static uint32_t debug_string(ObjDebug *debug, const char *text) {
    size_t at = 0;
    while (at < debug->string_size) {
        if (strcmp(debug->strings + at, text) == 0) return (uint32_t)at;
        at += strlen(debug->strings + at) + 1;
    }
    size_t length = strlen(text) + 1;
    debug->strings = realloc(debug->strings, debug->string_size + length);
    memcpy(debug->strings + debug->string_size, text, length);
    debug->string_size += length;
    return (uint32_t)at;
}

// Turn one text block's marks into trailer rows. `file`, `line` and `cost`
// carry across blocks: a statement's code can span several labels.
static void collect_debug(ObjDebug *debug, const AsmBlock *block, uint32_t block_pc,
                          uint32_t *file, uint32_t *line, uint32_t *cost) {
    for (size_t i = 0; i < block->mark_count; ++i) {
        const AsmMark *mark = &block->marks[i];
        uint32_t offset = block_pc + (uint32_t)mark->at * sizeof(uint32_t);
        if (mark->kind == ASM_MARK_FUNC) {
            *file = debug_string(debug, mark->file);
            *line = 0;
            *cost = 0;
            debug->funcs = realloc(debug->funcs, sizeof(ObjDebugFunc) * (debug->func_count + 1));
            debug->funcs[debug->func_count].offset = offset;
            debug->funcs[debug->func_count].name = debug_string(debug, mark->name);
            debug->funcs[debug->func_count].file = *file;
            debug->func_count++;
        } else if (mark->kind == ASM_MARK_LOC) {
            *line = mark->line;
        } else {
            *cost = debug_string(debug, mark->name);
        }
        // Directives at one address: the last says what the code there is.
        if (debug->row_count > 0 && debug->rows[debug->row_count - 1].offset == offset) {
            debug->row_count--;
        }
        debug->rows = realloc(debug->rows, sizeof(ObjDebugRow) * (debug->row_count + 1));
        debug->rows[debug->row_count].offset = offset;
        debug->rows[debug->row_count].file = *file;
        debug->rows[debug->row_count].line = *line;
        debug->rows[debug->row_count].cost = *cost;
        debug->row_count++;
    }
}

// TEXT, DATA and COLLECT: what codeGen lays out and writes per section.
#define SECTION_COUNT (SECTION_COLLECT + 1)

// What codeGen builds up as it goes through the blocks.
typedef struct {
    const char **imports;
    size_t import_count;
    const char **exports;
    size_t export_count;
    char module_name[32];          // tags this object's private labels
    uint32_t next_private;         // number of the next private label
    LabelMap labels;               // label -> offset in its block's section
    LabelSymbolMap symbol_names;   // label -> its name in the symbol table
    SymbolVec symbols;
    RelocVec relocs;
    ObjCollect *collects;
    size_t collect_count;
    ObjDebug debug;
    uint32_t size[SECTION_COUNT];  // bytes laid out in each section
} CodeGen;

static bool label_defined(CodeGen *cg, const char *label) {
    uint32_t addr;
    return getLabelAddress(&cg->labels, label, &addr);
}

// The name `label` has in the symbol table: the private name it was given,
// or itself.
static const char *symbol_name(const CodeGen *cg, const char *label) {
    const char *name = labelsymbolmap_get(&cg->symbol_names, label);
    return name ? name : label;
}

// A label this file refers to must be one it defines or imports. `where`
// adds what referred to it to the message.
static void require_symbol(CodeGen *cg, const char *label, const char *where) {
    if (label_defined(cg, label) || is_imported(label, cg->imports, cg->import_count)) return;
    fprintf(stderr, "Undefined symbol '%s'%s: not defined and not imported (add 'import %s')\n",
            label, where, label);
    exit(EXIT_FAILURE);
}

// Words are big-endian in memory, instructions included.
static void put_be32(uint8_t *out, uint32_t *at, uint32_t value) {
    out[(*at)++] = (uint8_t)((value >> 24) & 0xFF);
    out[(*at)++] = (uint8_t)((value >> 16) & 0xFF);
    out[(*at)++] = (uint8_t)((value >> 8) & 0xFF);
    out[(*at)++] = (uint8_t)(value & 0xFF);
}

// The bytes a block's `.byte` and `.word` directives emit: the bytes padded
// to a word boundary, then the words.
static uint32_t payload_size(const AsmBlock *block) {
    uint32_t padded_data = ((uint32_t)block->data_count + 3u) & ~3u;
    return padded_data + (uint32_t)block->word_count * sizeof(uint32_t);
}

// A `.data` or `.section` block holds data only.
static void check_data_only(const AsmBlock *block) {
    if (block->section == SECTION_TEXT || block->num_instrucitons == 0) return;
    if (block->section == SECTION_COLLECT) {
        fprintf(stderr, "'.section %s' block '%s' must hold only data (.byte/.word), not instructions\n",
                block->section_name, block->label);
    } else {
        fprintf(stderr, "'.data' block '%s' must hold only data (.byte/.word), not instructions\n",
                block->label);
    }
    exit(EXIT_FAILURE);
}

static void add_collect(CodeGen *cg, char *name, uint32_t offset, uint32_t size) {
    cg->collects = realloc(cg->collects, sizeof(ObjCollect) * (cg->collect_count + 1));
    cg->collects[cg->collect_count].name = name;
    cg->collects[cg->collect_count].offset = offset;
    cg->collects[cg->collect_count].size = size;
    cg->collect_count++;
}

// Put a block's label at `at` in the block's section, and in the symbol
// table under its own name if it is public -- an `f_` function, __START__,
// or exported -- or else under a name private to this object
// (PRIVATE_LABEL_FORMAT in ObjectFormat.h, which the linker recognises).
static void define_label(CodeGen *cg, const AsmBlock *block, uint32_t at) {
    mapLabelToAddress(&cg->labels, block->label, at);
    if (is_public_label(block->label) || is_exported(block->label, cg->exports, cg->export_count)) {
        labelsymbolmap_put(&cg->symbol_names, block->label, block->label);
        symbolvec_push(&cg->symbols, block->label, SYMBOL_DEFINED, block->section, at);
    } else {
        char unique_name[96];
        snprintf(unique_name, sizeof(unique_name), PRIVATE_LABEL_FORMAT, block->label, cg->module_name, cg->next_private++);
        labelsymbolmap_put(&cg->symbol_names, block->label, unique_name);
        symbolvec_push(&cg->symbols, unique_name, SYMBOL_DEFINED, block->section, at);
    }
}

// First pass: give every label its offset in its block's section -- TEXT,
// DATA, or the collected-section blob, which the linker places chunk by
// chunk -- and collect the symbols, the chunks and the debug rows.
static void lay_out_blocks(CodeGen *cg, AsmBlock *head) {
    uint32_t debug_file = 0, debug_line = 0, debug_cost = 0;
    for (AsmBlock *block = head; block; block = block->next) {
        if (!block->label || strlen(block->label) == 0) continue;
        check_data_only(block);
        uint32_t at = cg->size[block->section];
        uint32_t payload = payload_size(block);
        if (block->section == SECTION_COLLECT) add_collect(cg, block->section_name, at, payload);
        define_label(cg, block, at);
        if (block->section == SECTION_TEXT) {
            collect_debug(&cg->debug, block, at, &debug_file, &debug_line, &debug_cost);
        }
        cg->size[block->section] += (uint32_t)block->num_instrucitons * sizeof(uint32_t) + payload;
    }
}

static void check_exports(CodeGen *cg) {
    for (size_t i = 0; i < cg->export_count; i++) {
        if (!label_defined(cg, cg->exports[i])) {
            fprintf(stderr, "Exported symbol '%s' is not defined in this file\n", cg->exports[i]);
            exit(EXIT_FAILURE);
        }
    }
}

// Imports go in the symbol table as undefined, once each. An import has no
// section or offset: both are written as 0.
static void add_imports(CodeGen *cg) {
    for (size_t i = 0; i < cg->import_count; i++) {
        if (symbolvec_index(&cg->symbols, cg->imports[i]) >= 0) continue;
        symbolvec_push(&cg->symbols, cg->imports[i], SYMBOL_UNDEFINED, 0, 0);
    }
}

static uint8_t *alloc_section(uint32_t size, const char *failure) {
    uint8_t *bytes = malloc(size ? size : 1);
    if (!bytes) {
        perror(failure);
        exit(EXIT_FAILURE);
    }
    return bytes;
}

// Encode the instruction at `pc`. A label operand this file defines is
// resolved here: a jump's offset, or MOVI's address, which also gets an
// ABSOLUTE relocation because the linker moves the text. For an imported
// label the field is left 0 and the relocation says what goes there.
static uint32_t encode_at(CodeGen *cg, AsmInstr *inst, uint32_t pc) {
    if (inst->needs_fixup && inst->kind == INSTR_LABEL) {
        InstrLabel jump = inst->instruction->label;
        if (label_defined(cg, jump.label)) return encodeLabel(&cg->labels, jump, pc);
        require_symbol(cg, jump.label, "");
        relocvec_push(&cg->relocs, pc, jump.label, RELOC_RELATIVE, SECTION_TEXT);
        return ENCODE(jump.opcode, 26);
    }
    if (inst->needs_fixup && inst->kind == INSTR_REGLABEL) {
        const char *label = inst->instruction->reglabel.label;
        require_symbol(cg, label, "");
        uint32_t encoded = encodeInstruction(&cg->labels, inst, pc);
        relocvec_push(&cg->relocs, pc, symbol_name(cg, label), RELOC_ABSOLUTE, SECTION_TEXT);
        return encoded;
    }
    return encodeInstruction(&cg->labels, inst, pc);
}

// Write a block's `.byte` and `.word` payload at `*at` in `out`, its
// section. A word naming a symbol is written as 0 with a WORD32 relocation:
// the linker writes the symbol's final address there in full.
static void emit_payload(CodeGen *cg, const AsmBlock *block, uint8_t *out, uint32_t *at) {
    uint32_t padded_data = ((uint32_t)block->data_count + 3u) & ~3u;
    for (uint32_t i = 0; i < padded_data; ++i) {
        out[(*at)++] = i < block->data_count ? encodeByte(block->data[i]) : 0;
    }
    for (size_t i = 0; i < block->word_count; ++i) {
        const AsmWord *word = &block->words[i];
        uint32_t value = word->value;
        if (word->symbol) {
            require_symbol(cg, word->symbol, " in .word");
            relocvec_push(&cg->relocs, *at, symbol_name(cg, word->symbol), RELOC_WORD32, block->section);
            value = 0;
        }
        put_be32(out, at, value);
    }
}

// Second pass: encode every block's instructions into TEXT and write its
// payload to its own section.
static void emit_blocks(CodeGen *cg, AsmBlock *head, uint8_t *out[SECTION_COUNT]) {
    uint32_t at[SECTION_COUNT] = {0};
    for (AsmBlock *block = head; block; block = block->next) {
        for (AsmInstr *inst = block->inst_list; inst; inst = inst->next) {
            uint32_t encoded = encode_at(cg, inst, at[SECTION_TEXT]);
            put_be32(out[SECTION_TEXT], &at[SECTION_TEXT], encoded);
        }
        emit_payload(cg, block, out[block->section], &at[block->section]);
    }
}

MachineCode codeGen(AsmBlock *head, const char **imports, size_t import_count, const char **exports, size_t export_count, const char *module_tag) {
    CodeGen cg = {0};
    cg.imports = imports;
    cg.import_count = import_count;
    cg.exports = exports;
    cg.export_count = export_count;
    cg.next_private = 1;
    initLabelMap(&cg.labels);
    initLabelSymbolMap(&cg.symbol_names);
    build_module_tag(module_tag, cg.module_name, sizeof(cg.module_name));
    debug_string(&cg.debug, "");

    lay_out_blocks(&cg, head);
    check_exports(&cg);
    add_imports(&cg);

    uint8_t *out[SECTION_COUNT];
    out[SECTION_TEXT] = alloc_section(cg.size[SECTION_TEXT], "Failed to allocate machine code buffer");
    out[SECTION_COLLECT] = alloc_section(cg.size[SECTION_COLLECT], "Failed to allocate collected-section buffer");
    out[SECTION_DATA] = alloc_section(cg.size[SECTION_DATA], "Failed to allocate data section buffer");
    emit_blocks(&cg, head, out);

    MachineCode result = {0};
    result.code = out[SECTION_TEXT];
    result.size = cg.size[SECTION_TEXT];
    result.data = out[SECTION_DATA];
    result.data_size = cg.size[SECTION_DATA];
    result.symbols = cg.symbols.items;
    result.symbol_count = cg.symbols.count;
    result.relocs = cg.relocs.items;
    result.reloc_count = cg.relocs.count;
    result.collects = cg.collects;
    result.collect_count = cg.collect_count;
    result.blob = out[SECTION_COLLECT];
    result.blob_size = cg.size[SECTION_COLLECT];
    result.debug = cg.debug;
    return result;
}
