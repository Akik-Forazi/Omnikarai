CC     = gcc
CFLAGS = -Iinclude -Wall -Wextra -std=c99 -O2
LDFLAGS = -lkernel32

TARGET  = bin/omnicc.exe
SOURCES = src/main.c src/lexer.c src/parser.c src/codegen.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(SOURCES) | bin
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

bin:
	mkdir bin

clean:
	del /Q bin\omnicc.exe 2>NUL
	del /Q src\*.o 2>NUL

.PHONY: all clean
