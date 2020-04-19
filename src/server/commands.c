#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/helpers.h"

void checkout(int sock){
    char *project = set_create_project(sock, 0);
    if (!project)
        return;
    send_directory(sock, project);
}

void update(int sock){
    puts("Update");
}

void upgrade(int sock){
    puts("Upgrade");
}

void commit(int sock){
    char *project = set_create_project(sock, 0);
    if (!project)
        return;

    // send manifest
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + 1);
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);

    // receive client success msg on creating .Commit
    if (!recv_int(sock)){
        puts("Client encountered error when trying to .Commit");
        free(project);
        return;
    }

    // receive client .Commit file
    recv_file(sock, NULL);
    puts("Received new .Commit file");
    free(project);
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
    char *project = set_create_project(sock, 0);
    if (!project)
        return;

    char *cmd = malloc(strlen("rm -rf ") + strlen(project) + 1);
    sprintf(cmd, "rm -rf %s", project);
    system(cmd);
    free(project);
    free(cmd);
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
