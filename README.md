# MyAssembler Project

## Overview
MyAssembler is a 32-bit assembler designed to convert assembly language code into machine code based on a specified architecture. This project includes various modules for parsing, code generation, and utility functions, ensuring a structured approach to assembly language processing.

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


when updating instruction sets
check `lexer` `parser`
id integrate them for maintainabililty later

## Data directives

```asm
label:
  .byte 0x48, 0x69, 0x00          ; raw bytes, padded to a 4-byte boundary
  .word seven, 100, greeting      ; 32-bit big-endian words; a symbol is a
                                  ; placeholder the linker fills (RELOC_WORD32)

.section annotations              ; the next label block is one chunk of the
rows:                             ; named collected section: data only, no
  .word s_0, Counter__view, 12    ; instructions. The linker gathers all
                                  ; chunks of a name into an index between
                                  ; __annotations_start and __annotations_end.
```

Object files are LNK2 (`--obj`): see `toolchain/MyLinker/inc/ObjectFormat.h`
and `docs/design/toolchain-collected-sections.md`.
