CC = gcc
CFLAGS = -Iinc -Wall -Wextra -g -fPIE
SRC = $(wildcard src/*.c) $(wildcard src/**/*.c)

SRC_NO_MAIN = $(filter-out src/main.c, $(SRC))
OBJ = $(SRC:.c=.o)
OBJ_NO_MAIN = $(SRC_NO_MAIN:.c=.o)

TARGET = build/assembler

UNITY_SRC = tests/unity/unity.c
TEST_SRC = $(wildcard tests/test_*.c)
TEST_BIN = $(patsubst tests/%.c, build/%, $(TEST_SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p build
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	@for test in $(TEST_BIN); do \
		echo "Running $$test..."; \
		$$test; \
	done

build/%: tests/%.c $(UNITY_SRC) $(OBJ_NO_MAIN)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

run: $(TARGET)
	./$(TARGET)

gdb: $(TARGET)
	gdb ./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_BIN)
	rm -rf build

.PHONY: all clean test run gdb
