# MyAssembler Project

## Overview
MyAssembler is a 32-bit assembler designed to convert assembly language code into machine code based on a specified architecture. This project includes various modules for parsing, code generation, and utility functions, ensuring a structured approach to assembly language processing.

## Project Structure
```
MyAssembler
├── src
│   ├── main.c          # Entry point for the assembler
│   ├── assembler.c     # Implementation of assembler functions
│   ├── assembler.h     # Header for assembler functions
│   ├── parser.c        # Implementation of the parser
│   ├── parser.h        # Header for parser functions
│   ├── codegen.c       # Code generation logic
│   ├── codegen.h       # Header for code generation functions
│   └── utils
│       ├── file_utils.c # Utility functions for file operations
│       ├── file_utils.h # Header for file utility functions
│       ├── string_utils.c # Utility functions for string manipulation
│       └── string_utils.h # Header for string utility functions
├── include
│   └── architecture_spec.h # Architecture specification definitions
├── tests
│   ├── test_assembler.c   # Unit tests for the assembler
│   ├── test_parser.c      # Unit tests for the parser
│   └── test_codegen.c     # Unit tests for the code generation
├── Makefile                # Build instructions for the project
└── README.md               # Project documentation
```

## Setup Instructions
1. Clone the repository to your local machine.
2. Navigate to the project directory.
3. Run `make` to build the project. This will compile the source files and create the executable.

## Usage
To use the assembler, run the executable with the assembly file as an argument:
```
./myassembler <assembly_file.asm>
```
This will process the assembly file and generate the corresponding machine code.

## Architecture Specification
The assembler adheres to the architecture defined in `include/architecture_spec.h`. This file contains the necessary constants, structures, and function prototypes to ensure compliance with the architecture.

## Contribution
Contributions are welcome! Please submit a pull request or open an issue for any enhancements or bug fixes.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.