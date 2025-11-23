CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c99
TARGET=bin/omnicc
SOURCES=$(wildcard src/*.c)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: all clean
