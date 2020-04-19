.DEFAULT_GOAL := all

CC=gcc
CFLAGS=-lpthread -g -lcrypto

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# helpers
build/helpers.o: src/common/helpers.c src/common/helpers.h
	@$(CC) -c src/common/helpers.c -o build/helpers.o $(CFLAGS)

# server
build/server_commands.o: src/server/commands.c src/server/commands.h
	@$(CC) -c src/server/commands.c -o build/server_commands.o $(CFLAGS)

build/WTFserver.o: src/server/main.c
	@$(CC) -c src/server/main.c -o build/WTFserver.o $(CFLAGS)

# client
build/client_commands.o: src/client/commands.c src/client/commands.h
	@$(CC) -c src/client/commands.c -o build/client_commands.o $(CFLAGS)

build/WTF.o: src/client/main.c
	@$(CC) -c src/client/main.c -o build/WTF.o $(CFLAGS)

# link everything
bin/WTF: build/WTF.o build/client_commands.o build/helpers.o
	@$(CC) build/WTF.o build/client_commands.o build/helpers.o -o bin/WTF $(CFLAGS)

bin/WTFserver: build/WTFserver.o build/server_commands.o build/helpers.o
	@$(CC) build/WTFserver.o build/server_commands.o build/helpers.o -o bin/WTFserver $(CFLAGS)

all: bin/WTFserver bin/WTF

# runners

create: MANIFEST=tests_out/client/huffman_dir/.Manifest
create: all
	@./tests/scripts/create.sh 1>/dev/null
	@echo "c71b94505304dbad1526882c36fae444 ${MANIFEST}" | md5sum -c --quiet - && \
	 ( diff -qr tests_out/client/huffman_dir tests_out/server/huffman_dir && \
	 echo ${GREEN}PASS${NC} ) || echo ${RED}FAIL${NC}

add_remove: MANIFEST=tests_out/client/huffman_dir/.Manifest
add_remove: create
	@./tests/scripts/add_remove.sh 1>/dev/null
	@(echo "d3675e5d7625293a788597c22910c29c ${MANIFEST}" | md5sum -c --quiet -  && \
	echo ${GREEN}PASS${NC}) || echo ${RED}FAIL${NC}

currentversion: add_remove
	@(./tests/scripts/currentversion.sh 1>/dev/null && \
	echo ${GREEN}PASS${NC}) || echo ${RED}FAIL${NC}

run: currentversion

clean:
	$(RM) -r build/* bin/* .configure tests_out/server/* tests_out/client/* tests_out/client/.configure
