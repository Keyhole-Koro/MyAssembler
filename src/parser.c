#include "parser.h"

#define ERROR(cur, msg, ...) \
    error_with_context(__FILE__, __LINE__, cur, msg, ##__VA_ARGS__)

void error_with_context(const char *file, int line, Token *cur, const char *msg, ...) {
    fprintf(stderr, "Error at %s:%d: ", file, line);

    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    fprintf(stderr, "\nNext tokens:\n");
    int count = 0;
    while (cur && count++ < 5) {
        fprintf(stderr, "  [%d] type=%s, str='%s'\n", count, token_type_to_string(cur->type), cur->str);
        cur = cur->next;
    }

    exit(EXIT_FAILURE);
}
    

void consume(Token **cur) {
    if (*cur == NULL) {
        ERROR(*cur, "Unexpected end of tokens");
    }
    Token *next = (*cur)->next;
    *cur = next;
}

// slow
uint8_t mapOpcode(const char *opcode) {
    // 📥 Data Movement
    if (strcmp(opcode, "MOV") == 0)   return 0x01;
    if (strcmp(opcode, "MOVI") == 0)  return 0x02;
    if (strcmp(opcode, "MOVIS") == 0)  return 0x18;
    if (strcmp(opcode, "LD") == 0)    return 0x03;
    if (strcmp(opcode, "ST") == 0)    return 0x04;

    // ➕ Arithmetic / Logic
    if (strcmp(opcode, "ADD") == 0)   return 0x05;
    if (strcmp(opcode, "SUB") == 0)   return 0x06;
    if (strcmp(opcode, "CMP") == 0)   return 0x07;
    if (strcmp(opcode, "AND") == 0)   return 0x08;
    if (strcmp(opcode, "OR") == 0)    return 0x09;
    if (strcmp(opcode, "XOR") == 0)   return 0x0A;
    if (strcmp(opcode, "SHL") == 0)   return 0x0B;
    if (strcmp(opcode, "SHR") == 0)   return 0x0C;

    // ↺ Control Flow
    if (strcmp(opcode, "JMP") == 0)   return 0x0D;
    if (strcmp(opcode, "JZ") == 0)    return 0x0E;
    if (strcmp(opcode, "JNZ") == 0)   return 0x0F;
    if (strcmp(opcode, "JG") == 0)   return 0x10;
    if (strcmp(opcode, "JL") == 0)   return 0x11;
    if (strcmp(opcode, "JA") == 0)   return 0x12;
    if (strcmp(opcode, "JB") == 0)   return 0x13;

    // 📦 Stack
    if (strcmp(opcode, "PUSH") == 0)  return 0x14;
    if (strcmp(opcode, "POP") == 0)   return 0x15;

    // 🌐 I/O
    if (strcmp(opcode, "IN") == 0)    return 0x16;
    if (strcmp(opcode, "OUT") == 0)   return 0x17;

    // 🛑 Special
    if (strcmp(opcode, "HALT") == 0)  return 0x3F;

    fprintf(stderr, "Unknown opcode: %s\n", opcode);
    exit(EXIT_FAILURE);
}

uint8_t mapRegister(const char *reg) {
    if (strcmp(reg, "R0") == 0) return 0x00;
    if (strcmp(reg, "R1") == 0) return 0x01;
    if (strcmp(reg, "R2") == 0) return 0x02;
    if (strcmp(reg, "R3") == 0) return 0x03;
    if (strcmp(reg, "R4") == 0) return 0x04;
    if (strcmp(reg, "R5") == 0) return 0x05;
    if (strcmp(reg, "R6") == 0) return 0x06;
    if (strcmp(reg, "R7") == 0) return 0x07;
    if (strcmp(reg, "PC") == 0) return 0x08;
    if (strcmp(reg, "SP") == 0) return 0x09;
    if (strcmp(reg, "BP") == 0) return 0x0A;
    if (strcmp(reg, "SR") == 0) return 0x0B;
    if (strcmp(reg, "IR") == 0) return 0x0C;

    fprintf(stderr, "Unknown register: %s\n", reg);
    exit(EXIT_FAILURE);
}


InstructionList *instrNoOperand(Token *opcode) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrNoOperand *noOperand = malloc(sizeof(InstrNoOperand));
    instrList->kind = INSTR_NO_OPERAND;
    noOperand->opcode = mapOpcode(opcode->str);
    instrList->instruction = (Instruction *)noOperand;
    instrList->needs_fixup = false; // No fixup needed for no operand instructions
    instrList->next = NULL; // Initialize next pointer to NULL
    return (InstructionList *)instrList;
}

InstructionList *instrReg(Token *opcode, Token *reg1) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrReg *reg = malloc(sizeof(InstrReg));
    instrList->kind = INSTR_REG;
    reg->opcode = mapOpcode(opcode->str);
    reg->reg1 = mapRegister(reg1->str);
    instrList->instruction = (Instruction *)reg;
    instrList->needs_fixup = false; // No fixup needed for register instructions
    instrList->next = NULL; // Initialize next pointer to NULL
    return (InstructionList *)instrList;
}

InstructionList *instrRegReg(Token *opcode, Token *reg1, Token *reg2) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrRegReg *regReg = malloc(sizeof(InstrRegReg));
    instrList->kind = INSTR_REGREG;
    regReg->opcode = mapOpcode(opcode->str);
    regReg->reg1 = mapRegister(reg1->str);
    regReg->reg2 = mapRegister(reg2->str);
    instrList->instruction = (Instruction *)regReg;
    instrList->needs_fixup = false; // No fixup needed for register instructions
    instrList->next = NULL; // Initialize next pointer to NULL
    return (InstructionList *)instrList;
}

InstructionList *instrRegImm21(Token *opcode, Token *reg1, Token *imm21) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrRegImm21 *regImm21 = malloc(sizeof(InstrRegImm21));
    instrList->kind = INSTR_REGIMM21;
    regImm21->opcode = mapOpcode(opcode->str);
    regImm21->reg1 = mapRegister(reg1->str);

    if (imm21->type == NUMBER) {
        regImm21->imm21 = atoi(imm21->str); // Convert immediate value to integer
    }
    else if (imm21->type == NEGATIVE_NUMBER) {
        char signStr = imm21->str[0]; // Get the sign
        assert(signStr == '-'); // Ensure it's a negative number
        char *numStr = imm21->str + 1; // Skip the sign
        regImm21->imm21 = (~atoi(numStr) + 1) & 0x1FFFFF; // Convert to negative integer
    }
    instrList->instruction = (Instruction *)regImm21;
    instrList->needs_fixup = false; // No fixup needed for immediate instructions
    instrList->next = NULL; // Initialize next pointer to NULL
    return (InstructionList *)instrList;
}

InstructionList *instrRegAppears(Token **cur, Token *opcode, Token *reg1) {
    if ((*cur) && (*cur)->type == COMMA) {
        consume(cur);
        if ((*cur)->type == REGISTER) {
            Token *reg2 = *cur; // Save the second register
            consume(cur);
            return instrRegReg(opcode, reg1, reg2);

        } else if ((*cur)->type == NUMBER || (*cur)->type == NEGATIVE_NUMBER) {
            Token *imm21 = *cur; // Save the immediate value
            consume(cur);
            return instrRegImm21(opcode, reg1, imm21);
        
        
        } else {
            ERROR(*cur, "Expected REGISTER or NUMBER after COMMA but found: %s\n", (*cur)->str);
            exit(EXIT_FAILURE);
        }
    } else {
        return instrReg(opcode, reg1);
    }
}

InstructionList *instrLabel(Token *opcode, Token *label) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrLabel *instrlabel = malloc(sizeof(InstrLabel));
    instrList->kind = INSTR_LABEL;
    instrlabel->opcode = mapOpcode(opcode->str);
    instrlabel->label = label->str;
    instrList->instruction = (Instruction *)instrlabel;
    instrList->needs_fixup = true; // Fixup needed for label instructions
    instrList->next = NULL; // Initialize next pointer to NULL
    return (InstructionList *)instrList;
}

InstructionList *instrImm26(Token *opcode, Token *imm26) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrImm26 *imm26Instr = malloc(sizeof(InstrImm26));
    instrList->kind = INSTR_IMM26;
    imm26Instr->opcode = mapOpcode(opcode->str);
    imm26Instr->imm26 = atoi(imm26->str); // Convert immediate value to integer
    instrList->instruction = (Instruction *)imm26Instr;
    instrList->needs_fixup = false; // No fixup needed for immediate instructions
    instrList->next = NULL; // Initialize next pointer to NULL
    return (InstructionList *)instrList;
}

InstructionList *instructions(Token **cur) {
    if ((*cur)->type == INSTRUCTION) {
        Token *opcode = *cur;
        consume(cur);
        if (*cur && (*cur)->type == REGISTER) {
            Token *reg1 = *cur; // Save the first register
            consume(cur);
            return instrRegAppears(cur, opcode, reg1);
        } else if (*cur && (*cur)->type == NUMBER) {
            Token *imm26 = *cur;
            consume(cur);
            return instrImm26(opcode, imm26);
            
        } else if (*cur && (*cur)->type == LABEL
            // messy code
            // Check if the current token is a not label declaration
            && (*cur)->next != NULL
            && (*cur)->next->type != COLON) {

            Token *label = *cur; // Save the label
            consume(cur);
            return instrLabel(opcode, label);
    
        } else {
            return instrNoOperand(opcode);
        }
    } else {
        ERROR(*cur, "Unexpected token type for InstructionList: %s\n", token_type_to_string((*cur)->type));
        exit(EXIT_FAILURE);
    }
    
}

LabelInstructionLine *label(Token **cur) {
    Token *label = *cur;
    consume(cur);
    LabelInstructionLine *label_inst_line = malloc(sizeof(LabelInstructionLine));
    label_inst_line->label = label->str; // Store the label string
    label_inst_line->num_instrucitons = 0; // Initialize the number of instructions to 0
    label_inst_line->inst_list = NULL; // Initialize the list of instructions pointer to NULL
    label_inst_line->next = NULL; // Initialize the next pointer to NULL
    
    InstructionList *cur_inst = label_inst_line->inst_list;

    if ((*cur)->type == COLON) {
        consume(cur);
        if ((*cur) == NULL) return NULL; // Handle end of tokens gracefully

        while (*cur && (*cur)->type == INSTRUCTION && (*cur)->type != EOF) {

            InstructionList *new_inst = instructions(cur);
            label_inst_line->num_instrucitons++;
            if (label_inst_line->inst_list == NULL) {
                label_inst_line->inst_list = new_inst;
                cur_inst = new_inst;
            } else {
                cur_inst->next = new_inst;
                cur_inst = new_inst;
            }
        }
    } else {
        ERROR(*cur, "Expected ':' after %s but found: %s\n", label->str, (*cur)->str);
        exit(EXIT_FAILURE);
    }

    return label_inst_line;
}

LabelInstructionLine *parser(Token *head) {
    LabelInstructionLine *head_label_inst_line = NULL;
    LabelInstructionLine *cur_label_inst_line = NULL;
    cur_label_inst_line = head_label_inst_line;

    LabelInstructionLine *new_label = NULL;
    Token **cur = &head;
    while (*cur) {
        if ((*cur)->type == 0) consume(cur); // Skip

        if ((*cur)->type == LABEL) {
            new_label = label(cur);
        /*
        } else if ((*cur)->type == INSTRUCTION) {
            new_label = instructions(cur);
            */
        } else {
            ERROR(*cur, "Unexpected token type: %s\n", token_type_to_string((*cur)->type));
            exit(EXIT_FAILURE);
        }

        if (head_label_inst_line == NULL) {
            head_label_inst_line = new_label;
            cur_label_inst_line = new_label;
        } else {
            cur_label_inst_line->next = new_label;
            cur_label_inst_line = new_label;
        }
    }
    return head_label_inst_line;
}