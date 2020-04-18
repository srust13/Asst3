#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/helpers.h"

void checkout(int sock){
    puts("Checkout");
}

void update(int sock){
    puts("Update");
}

void upgrade(int sock){
    puts("Upgrade");
}

void commit(int sock){
    puts("Commit");
}

void push(int sock){
    puts("Push");
}

void create(int sock){
    // if project doesn't exist, create it
    // if project does exist, return because client made bad request
    char *project = set_create_project(sock, 1);
    if (!project)
        return;

    // send requested .Manifest to client
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 1);
    free(manifest);
    free(project);
}

void destroy(int sock){
    puts("Destroy");
}

void currentversion(int sock){
    // if project doesn't exist, client made bad request
    char *project = set_create_project(sock, 0);
    if (!project)
        return;

    // send requested .Manifest to client
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);
    free(project);
}

void history(int sock){
    puts("History");
}

void rollback(int sock){
    puts("Rollback");
}
