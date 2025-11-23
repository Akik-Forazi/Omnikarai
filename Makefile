CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c99
TARGET=bin/omnicc
SOURCES=$(wildcard src/*.c)

all: $(TARGET)

$(TARGET): $(SOURCES) | bin
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

bin:
	mkdir -p bin

clean:
	rm -f $(TARGET)

.PHONY: all clean
