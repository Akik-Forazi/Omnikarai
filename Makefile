CC = gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c99 -g
LLVM_CONFIG = llvm-config-15

# Get LLVM flags
LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs all --system-libs)

# Append LLVM flags
CFLAGS += $(LLVM_CFLAGS)

TARGET=bin/omnicc
OBJECTS=src/main.o src/lexer.o src/parser.o src/interpreter.o src/omni_runtime.o src/compiler.o src/jit_engine.o src/symbol_table.o

all: $(TARGET)

$(TARGET): $(OBJECTS) | bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LLVM_LDFLAGS) $(LLVM_LIBS)

# Rule to compile .c to .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p bin

clean:
	rm -f $(TARGET)
	rm -f src/*.o # Clean up object files
	# Clean up temporary Omnikarai generated files
	rm -f $(shell find . -name "*_omni_temp.c")
	rm -f $(shell find . -name "*_omni_temp.exe")

.PHONY: all clean
