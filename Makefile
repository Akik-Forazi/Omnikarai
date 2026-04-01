CC     = gcc
CFLAGS = -Iinclude -Wall -Wextra -std=c99 -O3 -march=native -mavx2 -mfma -ffast-math -DNDEBUG
LDFLAGS = -lkernel32 -lm

OMNICC  = bin/omnicc.exe
OMNIP   = bin/omnip.exe
SOURCES = src/main.c src/lexer.c src/parser.c src/codegen.c

all: $(OMNICC) $(OMNIP)

$(OMNICC): $(SOURCES) | bin
	$(CC) $(CFLAGS) -o $(OMNICC) $(SOURCES) $(LDFLAGS)

$(OMNIP): omnip/src/omnip.c | bin
	$(CC) -Wall -O3 -march=native -o $(OMNIP) omnip/src/omnip.c -lkernel32 -lwinhttp

bin:
	mkdir bin

clean:
	del /Q bin\omnicc.exe 2>NUL
	del /Q bin\omnip.exe 2>NUL

.PHONY: all clean
