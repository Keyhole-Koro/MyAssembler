#include "utils.h"
#include "parser.h"
#include "instructions.h"

#include <string.h>

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case COLON:       return "COLON";
        case COMMA:       return "COMMA";
        case PERIOD:      return "PERIOD";
        case IDENTIFIER:  return "IDENTIFIER";
        case INSTRUCTION: return "INSTRUCTION";
        case REGISTER:    return "REGISTER";
        case NUMBER:      return "NUMBER";
        case LABEL:       return "LABEL";
        case NEWLINE:     return "NEWLINE";
        default:          return "UNKNOWN";
    }
}


void print_binary32(uint32_t value) {
    for (int i = 31; i >= 0; --i) {
        printf("%c", (value >> i) & 1 ? '1' : '0');
        if (i % 4 == 0 && i != 0) printf(" "); // 4ビット区切り
    }
}

static const char* mnemonic_from_opcode(uint8_t opcode) {
    for (size_t i = 0; i < instruction_count; ++i) {
        if (instruction_table[i].opcode == opcode) return instruction_table[i].name;
    }
    return "?";
}

static const char* reg_name(uint8_t code) {
    switch (code) {
        case 0x00: return "r0";
        case 0x01: return "r1";
        case 0x02: return "r2";
        case 0x03: return "r3";
        case 0x04: return "r4";
        case 0x05: return "r5";
        case 0x06: return "r6";
        case 0x07: return "r7";
        case 0x08: return "pc";
        case 0x09: return "sp";
        case 0x0A: return "bp";
        case 0x0B: return "sr";
        case 0x0C: return "lr";
        default: return "r?";
    }
}

static void print_instruction_list(const InstructionList *inst_list, int indent) {
    for (const InstructionList *n = inst_list; n; n = n->next) {
        const Instruction *inst = n->instruction;
        const char *mn = mnemonic_from_opcode(inst->base.opcode);
        for (int i = 0; i < indent; ++i) putchar(' ');
        printf("- %s ", mn);
        switch (n->kind) {
            case INSTR_REGREG:
                printf("%s, %s", reg_name(inst->regreg.reg1), reg_name(inst->regreg.reg2));
                break;
            case INSTR_REGIMM21:
                printf("%s, %u", reg_name(inst->regimm21.reg1), inst->regimm21.imm21 & 0x1FFFFF);
                break;
            case INSTR_REG:
                printf("%s", reg_name(inst->reg.reg1));
                break;
            case INSTR_LABEL:
                printf("%s", inst->label.label);
                break;
            case INSTR_IMM26:
                printf("#%u", inst->imm26.imm26 & 0x3FFFFFF);
                break;
            case INSTR_NO_OPERAND:
                // nothing
                break;
            case INSTR_REGLABEL:
                printf("%s, %s", reg_name(inst->reglabel.reg1), inst->reglabel.label);
                break;
            default:
                printf("<unknown kind>");
                break;
        }
        if (n->needs_fixup) printf("  [fixup]");
        printf("\n");
    }
}

void print_label_instruction_lines(const LabelInstructionLine *head) {
    // Delegate to file-based implementation for consistency
    fprint_label_instruction_lines(stdout, head);
}

static void fprint_instruction_list(FILE *out, const InstructionList *inst_list, int indent) {
    for (const InstructionList *n = inst_list; n; n = n->next) {
        const Instruction *inst = n->instruction;
        const char *mn = mnemonic_from_opcode(inst->base.opcode);
        for (int i = 0; i < indent; ++i) fputc(' ', out);
        fprintf(out, "- %s ", mn);
        switch (n->kind) {
            case INSTR_REGREG:
                fprintf(out, "%s, %s", reg_name(inst->regreg.reg1), reg_name(inst->regreg.reg2));
                break;
            case INSTR_REGIMM21:
                fprintf(out, "%s, %u", reg_name(inst->regimm21.reg1), inst->regimm21.imm21 & 0x1FFFFF);
                break;
            case INSTR_REG:
                fprintf(out, "%s", reg_name(inst->reg.reg1));
                break;
            case INSTR_LABEL:
                fprintf(out, "%s", inst->label.label);
                break;
            case INSTR_IMM26:
                fprintf(out, "#%u", inst->imm26.imm26 & 0x3FFFFFF);
                break;
            case INSTR_NO_OPERAND:
                // nothing
                break;
            case INSTR_REGLABEL:
                fprintf(out, "%s, %s", reg_name(inst->reglabel.reg1), inst->reglabel.label);
                break;
            default:
                fprintf(out, "<unknown kind>");
                break;
        }
        if (n->needs_fixup) fprintf(out, "  [fixup]");
        fprintf(out, "\n");
    }
}

void fprint_label_instruction_lines(FILE *out, const LabelInstructionLine *head) {
    if (!out) return;
    fprintf(out, "Program:\n");
    for (const LabelInstructionLine *line = head; line; line = line->next) {
        fprintf(out, "%s:\n", line->label ? line->label : "<no-label>");
        if (line->inst_list) {
            fprint_instruction_list(out, line->inst_list, 2);
        }
        if (line->data_count > 0) {
            fprintf(out, "  .byte ");
            for (size_t i = 0; i < line->data_count; ++i) {
                fprintf(out, "%s0x%02X", (i ? ", " : ""), (unsigned)line->data[i]);
            }
            fprintf(out, "\n");
        }
    }
}

void fprint_tokens(FILE *out, const Token *tokens) {
    if (!out) return;
    for (const Token *t = tokens; t != NULL; t = t->next) {
        fprintf(out, "Token: Type=%s, Str='%s'\n", token_type_to_string(t->type), t->str);
    }
}

void print_tokens(const Token *tokens) {
    fprint_tokens(stdout, tokens);
}

void write_debug_report(const char *path, const Token *tokens, const LabelInstructionLine *program) {
    if (!path) return;
    FILE *out = fopen(path, "w");
    if (!out) {
        perror("Error opening output file");
        return;
    }
    fprintf(out, "Tokens:\n");
    fprint_tokens(out, tokens);
    fprintf(out, "\n");
    fprint_label_instruction_lines(out, program);
    fclose(out);
}
