.DEFAULT_GOAL := all

CC=gcc
CFLAGS=-lpthread

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# server
bin/WTFserver: src/server/main.c
	$(CC) src/server/main.c -o bin/WTFserver $(CFLAGS)

# client
build/client_helpers.o: src/client/helpers.c src/client/helpers.h
	$(CC) -c src/client/helpers.c -o build/client_helpers.o $(CFLAGS)

build/client_commands.o: src/client/commands.c src/client/commands.h
	$(CC) -c src/client/commands.c -o build/client_commands.o $(CFLAGS)

build/WTF.o: src/client/main.c
	$(CC) -c src/client/main.c -o build/WTF.o $(CFLAGS)

# link everything
bin/WTF: build/WTF.o build/client_commands.o build/client_helpers.o
	$(CC) build/WTF.o build/client_commands.o build/client_helpers.o -o bin/WTF $(CFLAGS)

all: bin/WTFserver bin/WTF

# runners
# run: all
# 	./bin/WTFserver

clean:
	$(RM) build/* bin/* .configure
