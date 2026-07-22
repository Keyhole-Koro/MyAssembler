CC = gcc
CFLAGS = -Iinc -I../MyLinker/inc -Wall -Wextra -g -fPIE
SRC = $(wildcard src/*.c) $(wildcard src/**/*.c)

SRC_NO_MAIN = $(filter-out src/main.c, $(SRC))
OBJ = $(SRC:.c=.o)
OBJ_NO_MAIN = $(SRC_NO_MAIN:.c=.o)

TARGET = build/myas

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

test-e2e: $(TARGET)
	python3 tests/run_integration_tests.py

# Backward-compatible alias. This suite uses the linker and emulator.
test-integration: test-e2e

# Tests owned by and executable within the assembler repository.
test-component: test

# Developer convenience aggregate. Repository CI runs test-component;
# MyComputer runs test-e2e against its pinned toolchain revisions.
test-all: test-component test-e2e

build/%: tests/%.c $(UNITY_SRC) $(OBJ_NO_MAIN)
	mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

run-myas: $(TARGET)
	./$(TARGET) $(IN) $(OUT)

gdb: $(TARGET)
	gdb ./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_BIN)
	rm -rf build

.PHONY: all clean test test-component test-e2e test-integration test-all run-myas gdb
