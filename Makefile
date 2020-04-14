.DEFAULT_GOAL := all

CC=gcc
CFLAGS=-lpthread -g

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# server

build/server_helpers.o: src/server/helpers.c src/server/helpers.h
	@$(CC) -c src/server/helpers.c -o build/server_helpers.o $(CFLAGS)

build/server_commands.o: src/server/commands.c src/server/commands.h
	@$(CC) -c src/server/commands.c -o build/server_commands.o $(CFLAGS)

build/WTFserver.o: src/server/main.c
	@$(CC) -c src/server/main.c -o build/WTFserver.o $(CFLAGS)

# client
build/client_helpers.o: src/client/helpers.c src/client/helpers.h
	@$(CC) -c src/client/helpers.c -o build/client_helpers.o $(CFLAGS)

build/client_commands.o: src/client/commands.c src/client/commands.h
	@$(CC) -c src/client/commands.c -o build/client_commands.o $(CFLAGS)

build/WTF.o: src/client/main.c
	@$(CC) -c src/client/main.c -o build/WTF.o $(CFLAGS)

# link everything
bin/WTF: build/WTF.o build/client_commands.o build/client_helpers.o
	@$(CC) build/WTF.o build/client_commands.o build/client_helpers.o -o bin/WTF $(CFLAGS)

bin/WTFserver: build/WTFserver.o build/server_commands.o build/server_helpers.o
	@$(CC) build/WTFserver.o build/server_commands.o build/server_helpers.o -o bin/WTFserver $(CFLAGS)

all: bin/WTFserver bin/WTF

# runners

create: MANIFEST=test_out/client/huffman_dir/.Manifest
create: all
	@./tests/scripts/create.sh
	@echo "4a9557b5bbe09ae19ffed20aa64d2c78 ${MANIFEST}" | md5sum -c --quiet - && \
	 ( diff -qr test_out/client/huffman_dir test_out/server/huffman_dir && \
	 echo ${GREEN}PASS${NC} ) || echo ${RED}FAIL${NC}

run: create

clean:
	$(RM) -r build/* bin/* .configure test_out/server/* test_out/client/* test_out/client/.configure
