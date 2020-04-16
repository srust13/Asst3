#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "commands.h"

int sock;

void configure(char *hostname, char *port){
    // create data
    int buf_len = strlen(hostname) + strlen(port) + 2;
    char *buf = malloc(buf_len + 1); // sprintf adds null terminator
    sprintf(buf, "%s\n%s\n", hostname, port);

    // write data to file
    int fd = open(".configure", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    write(fd, buf, buf_len);
    close(fd);
    free(buf);
}

void checkout(char *project){
    puts("Checkout");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void update(char *project){
    puts("Update");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void upgrade(char *project){
    puts("Upgrade");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void commit(char *project){
    puts("Commit");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void push(char *project){
    puts("Push");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void create(char *project){
    set_socket(&sock);
    sendline_ack(sock, "create");

    // make sure project doesn't exist on server
    int proj_exists = server_project_exists(sock, project);
    if (proj_exists){
        puts("Project already exists on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // create local project folder
    mkdir(project, 0755);

    // receive remote .Manifest file
    char *fname = malloc(strlen(project) + strlen(".Manifest") + 2);
    sprintf(fname, "%s/.Manifest", project);
    recv_file(sock, fname);

    // cleanup
    free(fname);
    close(sock);
    puts("Client gracefully disconnected from server");
}

void destroy(char *project){
    puts("Destroy");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void add(char *project, char *filename){

    // check if project exists locally
    int success = 0;
    struct stat st = {0};
    if (stat(project, &st) == -1){
        puts("Local project does not exist");
        exit(EXIT_FAILURE);
    }

    // create manifest path
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + 2);
    sprintf(manifest, "%s/.Manifest", project);

    // hash file
    char hexstring[33];
    md5sum(filename, hexstring);

    // create buf
    int buf_size = strlen(filename) + strlen(hexstring) + strlen("+++0  \n ");
    char *data = malloc(buf_size);
    sprintf(data, "+++0 %s %s\n", filename, hexstring);

    // write "new file" indicator
    int fd = open(manifest, O_RDWR | O_APPEND);
    write(fd, data, buf_size-1); // no null byte

    // cleanup
    close(fd);
    free(data);
    free(manifest);
}

void remove_cmd(char *project, char *filename){
    // check if project exists locally
    int success = 0;
    struct stat st = {0};
    if (stat(project, &st) == -1){
        puts("Local project does not exist");
        exit(EXIT_FAILURE);
    }

    // create manifest path
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + 2);
    sprintf(manifest, "%s/.Manifest", project);

    // todo: edit in middle of file by removing line that has filename in it
    // read_chunks()

    // cleanup
    free(manifest);
}

void currentversion(char *project){
    puts("CurrentVersion");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void history(char *project){
    puts("History");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void rollback(char *project, char *version){
    puts("Rollback");
    printf("Project: %s\n", project);
    printf("Version: %s\n", version);

    set_socket(&sock);
    close(sock);
}
