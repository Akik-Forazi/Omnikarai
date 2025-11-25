CC = gcc # Changed from gcc to clang
CFLAGS=-Iinclude -Wall -Wextra -std=c99 -g # Added -g for debug symbols
TARGET=bin/omnicc
# SOURCES=$(wildcard src/*.c) # This already includes compiler.c and omni_runtime.c

# Explicitly list object files to ensure all new sources are compiled
OBJECTS=src/main.o src/lexer.o src/parser.o src/compiler.o src/omni_runtime.o

all: $(TARGET)

$(TARGET): $(OBJECTS) | bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

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
