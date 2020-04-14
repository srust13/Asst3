#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "helpers.h"

int sock;

void configure(char *hostname, char *port){
    // create buffer
    int buf_len = strlen(hostname) + strlen(port) + 2;
    char *buf = malloc(buf_len + 1); // sprintf adds null terminator
    sprintf(buf, "%s\n%s\n", hostname, port);

    // write to file
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
    send_msg(sock, "create");
    send_msg(sock, project);

    // make sure project doesn't exist on server
    int success = 0;
    recv(sock, &success, sizeof(success), MSG_WAITALL);
    if (!success){
        puts("Project already exists on server");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // create local project folder
    mkdir(project, 0755);

    // open local .Manifest file
    char *fname = malloc(strlen(project) + strlen(".Manifest") + 2);
    sprintf(fname, "%s/.Manifest", project);
    int manifest_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(fname);

    // receive bytes to write to .Manifest file
    int file_size = 0;
    recv(sock, &file_size, sizeof(file_size), MSG_WAITALL);
    file_size = ntohl(file_size);
    char *manifest_data = malloc(CHUNK_SIZE);
    while (file_size > 0){
        int bytes_read = recv(sock, manifest_data, CHUNK_SIZE, 0);
        file_size -= bytes_read;
        write(manifest_fd, manifest_data, bytes_read);
    }

    // cleanup
    free(manifest_data);
    close(manifest_fd);
    close(sock);
}

void destroy(char *project){
    puts("Destroy");
    printf("Project: %s\n", project);

    set_socket(&sock);
    close(sock);
}

void add(char *project, char *filename){
    puts("Add");
    printf("Project: %s\n", project);
    printf("Filename: %s\n", filename);
}

void remove_cmd(char *project, char *filename){
    puts("Remove");
    printf("Project: %s\n", project);
    printf("Filename: %s\n", filename);
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
