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

# tests

create: all
	@(./tests/scripts/create.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} create ) || /bin/echo -e ${RED}FAIL${NC} create

add_remove: create
	@(./tests/scripts/add_remove.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} add_remove ) || /bin/echo -e ${RED}FAIL${NC} add_remove

currentversion: add_remove
	@(./tests/scripts/currentversion.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} currentversion) || /bin/echo -e ${RED}FAIL${NC} currentversion

checkout: add_remove
	@(./tests/scripts/checkout.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} checkout) || /bin/echo -e ${RED}FAIL${NC} checkout

destroy: checkout
	@(./tests/scripts/destroy.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} destroy) || /bin/echo -e ${RED}FAIL${NC} destroy

commit: add_remove
	@(./tests/scripts/commit.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} commit) || /bin/echo -e ${RED}FAIL${NC} commit

push: commit
	@(./tests/scripts/push.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} push) || /bin/echo -e ${RED}FAIL${NC} push

update: push
	@(./tests/scripts/update.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} update) || /bin/echo -e ${RED}FAIL${NC} update

upgrade: update
	@(./tests/scripts/upgrade.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} upgrade) || /bin/echo -e ${RED}FAIL${NC} upgrade

rollback: push
	@(./tests/scripts/rollback.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} rollback) || /bin/echo -e ${RED}FAIL${NC} rollback

history: upgrade
	@(./tests/scripts/history.sh 1>/dev/null && \
	/bin/echo -e ${GREEN}PASS${NC} history) || /bin/echo -e ${RED}FAIL${NC} history

run: currentversion destroy rollback history

clean:
	$(RM) -r build/* bin/* .configure tests_out/server/* tests_out/client/* tests_out/client/.configure tests_out/client2
