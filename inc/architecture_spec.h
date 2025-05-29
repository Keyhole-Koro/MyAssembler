#ifndef ARCHITECTURE_SPEC_H
#define ARCHITECTURE_SPEC_H

// Define constants for the architecture
#define WORD_SIZE 4 // 32-bit architecture
#define MAX_INSTRUCTIONS 256
#define MAX_LABEL_LENGTH 32
#define MAX_OPERAND_LENGTH 16

// Define instruction formats
typedef enum {
    INSTRUCTION_TYPE_R,
    INSTRUCTION_TYPE_I,
    INSTRUCTION_TYPE_J
} InstructionType;

// Structure for an instruction
typedef struct {
    InstructionType type;
    char opcode[MAX_OPERAND_LENGTH];
    char operands[3][MAX_OPERAND_LENGTH]; // Up to 3 operands
} Instruction;

// Function prototypes
void initialize_architecture();
void validate_instruction(const Instruction *instr);
void encode_instruction(const Instruction *instr, unsigned char *output);

#endif // ARCHITECTURE_SPEC_H