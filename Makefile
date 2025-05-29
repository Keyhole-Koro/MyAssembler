CC = gcc
CFLAGS = -Iinc -Wall -Wextra -g
SRC = $(wildcard src/*.c) $(wildcard src/**/*.c)
OBJ = $(SRC:.c=.o)
TARGET = assembler

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./tests/test_assembler
	./tests/test_parser
	./tests/test_codegen

run: $(TARGET)
	./$(TARGET)

gdb: $(TARGET)
	gdb ./$(TARGET)
	
clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean test run