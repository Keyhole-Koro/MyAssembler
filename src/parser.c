#include "parser.h"

#define ERROR(msg, ...) \
    fprintf(stderr, "Error at %s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__), exit(EXIT_FAILURE)
    

void consume(Token **cur) {
    if (*cur == NULL) {
        ERROR("Unexpected end of tokens");
    }
    Token *next = (*cur)->next;
    *cur = next;
}

// slow
uint8_t mapOpcode(const char *opcode) {
    // 📥 Data Movement
    if (strcmp(opcode, "MOV") == 0)   return 0x00;
    if (strcmp(opcode, "MOVI") == 0)  return 0x01;
    if (strcmp(opcode, "LD") == 0)    return 0x02;
    if (strcmp(opcode, "ST") == 0)    return 0x03;

    // ➕ Arithmetic/Logic
    if (strcmp(opcode, "ADD") == 0)   return 0x04;
    if (strcmp(opcode, "SUB") == 0)   return 0x05;
    if (strcmp(opcode, "CMP") == 0)   return 0x06;
    if (strcmp(opcode, "AND") == 0)   return 0x07;
    if (strcmp(opcode, "OR") == 0)    return 0x08;
    if (strcmp(opcode, "XOR") == 0)   return 0x09;
    if (strcmp(opcode, "SHL") == 0)   return 0x0A;
    if (strcmp(opcode, "SHR") == 0)   return 0x0B;

    // 🔁 Control Flow
    if (strcmp(opcode, "JMP") == 0)   return 0x0C;
    if (strcmp(opcode, "JZ") == 0)    return 0x0D;
    if (strcmp(opcode, "JNZ") == 0)   return 0x0E;
    if (strcmp(opcode, "CALL") == 0)  return 0x0F;
    if (strcmp(opcode, "RET") == 0)   return 0x10;

    // 📦 Stack
    if (strcmp(opcode, "PUSH") == 0)  return 0x11;
    if (strcmp(opcode, "POP") == 0)   return 0x12;

    // 🌐 I/O
    if (strcmp(opcode, "IN") == 0)    return 0x13;
    if (strcmp(opcode, "OUT") == 0)   return 0x14;

    // 🛑 Special
    if (strcmp(opcode, "HALT") == 0)  return 0x1F;

    ERROR("Unknown opcode: %s\n", opcode);
    exit(EXIT_FAILURE);
}

InstructionList *instrNoOperand(Token *opcode) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrNoOperand *noOperand = malloc(sizeof(InstrNoOperand));
    noOperand->opcode = mapOpcode(opcode->str);
    instrList->instruction = (Instruction *)noOperand;
    instrList->needs_fixup = false; // No fixup needed for no operand instructions
    return (InstructionList *)instrList;
}

InstructionList *instrReg(Token *opcode, Token *reg1) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrReg *reg = malloc(sizeof(InstrReg));
    reg->opcode = mapOpcode(opcode->str);
    reg->reg1 = atoi(reg1->str + 1); // Assuming register names are like R0, R1, etc.
    instrList->instruction = (Instruction *)reg;
    instrList->needs_fixup = false; // No fixup needed for register instructions
    return (InstructionList *)instrList;
}

InstructionList *instrRegReg(Token *opcode, Token *reg1, Token *reg2) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrRegReg *regReg = malloc(sizeof(InstrRegReg));
    regReg->opcode = mapOpcode(opcode->str);
    regReg->reg1 = atoi(reg1->str + 1); // Assuming register names are like R0, R1, etc.
    regReg->reg2 = atoi(reg2->str + 1); // Assuming register names are like R0, R1, etc.
    instrList->instruction = (Instruction *)regReg;
    instrList->needs_fixup = false; // No fixup needed for register instructions
    return (InstructionList *)instrList;
}

InstructionList *instrRegImm21(Token *opcode, Token *reg1, Token *imm21) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrRegImm21 *regImm21 = malloc(sizeof(InstrRegImm21));
    regImm21->opcode = mapOpcode(opcode->str);
    regImm21->reg1 = atoi(reg1->str + 1); // Assuming register names are like R0, R1, etc.
    regImm21->imm21 = atoi(imm21->str); // Convert immediate value to integer
    instrList->instruction = (Instruction *)regImm21;
    instrList->needs_fixup = false; // No fixup needed for immediate instructions
    return (InstructionList *)instrList;
}

InstructionList *instrRegAppears(Token **cur, Token *opcode, Token *reg1) {
    if ((*cur)->type == COMMA) {
        consume(cur);
        if ((*cur)->type == REGISTER) {
            Token *reg2 = *cur; // Save the second register
            consume(cur);
            return instrRegReg(opcode, reg1, reg2);

        } else if ((*cur)->type == NUMBER) {
            Token *imm21 = *cur; // Save the immediate value
            consume(cur);
            return instrRegImm21(opcode, reg1, imm21);
        
        
        } else {
            ERROR("Expected REGISTER or NUMBER after COMMA but found: %s\n", (*cur)->str);
            exit(EXIT_FAILURE);
        }
    } else {
        return instrReg(opcode, reg1);
    }
}

InstructionList *instrLabel(Token *opcode, Token *label) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrLabel *instrlabel = malloc(sizeof(InstrLabel));
    instrlabel->opcode = mapOpcode(opcode->str);
    instrlabel->label = label->str;
    instrList->instruction = (Instruction *)instrlabel;
    instrList->needs_fixup = true; // Fixup needed for label instructions
    return (InstructionList *)instrList;
}

InstructionList *instrImm26(Token *opcode, Token *imm26) {
    InstructionList *instrList = malloc(sizeof(InstructionList));
    InstrImm26 *imm26Instr = malloc(sizeof(InstrImm26));
    imm26Instr->opcode = mapOpcode(opcode->str);
    imm26Instr->imm26 = atoi(imm26->str); // Convert immediate value to integer
    instrList->instruction = (Instruction *)imm26Instr;
    instrList->needs_fixup = false; // No fixup needed for immediate instructions
    return (InstructionList *)instrList;
}

InstructionList *instructions(Token **cur) {
    if ((*cur)->type == INSTRUCTION) {
        Token *opcode = *cur;
        consume(cur);
        if ((*cur) == NULL) return NULL; // Handle end of tokens gracefully
        if ((*cur)->type == REGISTER) {
            Token *reg1 = *cur; // Save the first register
            consume(cur);
            return instrRegAppears(cur, opcode, reg1);
        } else if ((*cur)->type == NUMBER) {
            Token *imm26 = *cur;
            consume(cur);
            return instrImm26(opcode, imm26);
            
        } else if ((*cur)->type == LABEL
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
        ERROR("Unexpected token type for InstructionList: %d\n", (*cur)->type);
        exit(EXIT_FAILURE);
    }
    
}

LabelInstructionLine *label(Token **cur) {
    Token *label = *cur;
    consume(cur);
    LabelInstructionLine *label_inst_line = malloc(sizeof(LabelInstructionLine));
    label_inst_line->label = label->str; // Store the label string
    label_inst_line->inst_list = NULL; // Initialize the list of instructions pointer to NULL
    
    InstructionList *cur_inst = label_inst_line->inst_list;

    if ((*cur)->type == COLON) {
        consume(cur);
        if ((*cur) == NULL) return NULL; // Handle end of tokens gracefully

        while (*cur && (*cur)->type == INSTRUCTION && (*cur)->type != EOF) {

            InstructionList *new_inst = instructions(cur);
            if (label_inst_line->inst_list == NULL) {
                label_inst_line->inst_list = new_inst;
                cur_inst = new_inst;
            } else {
                cur_inst->next = new_inst;
                cur_inst = new_inst;
            }
        }
    } else {
        ERROR("Expected ':' after label but found: %s\n", (*cur)->str);
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
            ERROR("Unexpected token type: %d\n", (*cur)->type);
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