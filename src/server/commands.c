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
    char *project = set_create_project(sock, 0);
    if (!project)
        return;

    // receive client's .Commit
    char temp_commit[15+1];
    gen_temp_filename(temp_commit);
    recv_file(sock, temp_commit);

    // TODO: check if any client commit for this project exists and matches the received one
    int matches = 1;

    // if received commit has expired; inform client and close connection
    if (!matches){
        send_int(sock, 0);
        remove(temp_commit);
        return;
    }
    send_int(sock, 1);

    // TODO: Expire other .Commit files for this project

    // receive tar with changed files
    char temp_tar[15+1];
    gen_temp_filename(temp_tar);
    recv_file(sock, temp_tar);

    remove(temp_tar); // TODO: Remove these 2 lines
    return;

    // untar files into project
    char *untar_cmd = malloc(strlen("tar xzf ") + strlen(temp_tar) + 1);
    sprintf(untar_cmd, "tar xzf %s", temp_tar);
    system(untar_cmd);
    free(untar_cmd);

    // TODO: regenerate manifest from .Commit, then expire the final commit

    // cleanup
    remove(temp_commit);
    remove(temp_tar);
    free(project);
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
