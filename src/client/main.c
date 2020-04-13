#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "commands.h"

const char *usage_str =
"\nusage: wtf <command> [<args>]\n\n"
"commands:\n"
"    configure      <hostname> <port>\n"
"    checkout       <project>\n"
"    update         <project>\n"
"    upgrade        <project>\n"
"    commit         <project>\n"
"    push           <project>\n"
"    create         <project>\n"
"    destroy        <project>\n"
"    add            <project> <filename>\n"
"    remove         <project> <filename>\n"
"    currentversion <project>\n"
"    history        <project>\n"
"    rollback       <project> <version>";

void usage(char *msg){
    if (msg) puts(msg);
    puts(usage_str);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){

    if (argc < 3) usage("Unreconized command or missing argument");
    char *cmd = argv[1];

    if (!strcmp(cmd, "configure")){
        if (argc < 4) usage("Missing port arg for configure");
        configure(argv[2], argv[3]);
    } else if (!strcmp(cmd, "checkout")){
        checkout(argv[2]);
    } else if (!strcmp(cmd, "update")){
        update(argv[2]);
    } else if (!strcmp(cmd, "upgrade")){
        upgrade(argv[2]);
    } else if (!strcmp(cmd, "commit")){
        commit(argv[2]);
    } else if (!strcmp(cmd, "push")){
        push(argv[2]);
    } else if (!strcmp(cmd, "create")){
        create(argv[2]);
    } else if (!strcmp(cmd, "destroy")){
        destroy(argv[2]);
    } else if (!strcmp(cmd, "add")){
        if (argc < 4) usage("Missing filename args for add");
        add(argv[2], argv[3]);
    } else if (!strcmp(cmd, "remove")){
        if (argc < 4) usage("Missing filename args for remove");
        remove_cmd(argv[2], argv[3]);
    } else if (!strcmp(cmd, "currentversion")){
        currentversion(argv[2]);
    } else if (!strcmp(cmd, "history")){
        history(argv[2]);
    } else if (!strcmp(cmd, "rollback")){
        if (argc < 4) usage("Missing version arg for rollback");
        rollback(argv[2], argv[3]);
    } else {
        usage("Invalid command");
    }
    return 0;
}
