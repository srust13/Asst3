#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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

    read_and_test_config(&sock);
    close(sock);
}

void update(char *project){
    puts("Update");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
    close(sock);
}

void upgrade(char *project){
    puts("Upgrade");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
    close(sock);
}

void commit(char *project){
    puts("Commit");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
    close(sock);
}

void push(char *project){
    puts("Push");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
    close(sock);
}

void create(char *project){
    puts("Create");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
    close(sock);
}

void destroy(char *project){
    puts("Destroy");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
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

    read_and_test_config(&sock);
    close(sock);
}

void history(char *project){
    puts("History");
    printf("Project: %s\n", project);

    read_and_test_config(&sock);
    close(sock);
}

void rollback(char *project, char *version){
    puts("Rollback");
    printf("Project: %s\n", project);
    printf("Version: %s\n", version);

    read_and_test_config(&sock);
    close(sock);
}
