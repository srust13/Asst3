#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/helpers.h"

void checkout(buf_socket_t *conn){
    puts("Checkout");
}

void update(buf_socket_t *conn){
    puts("Update");
}

void upgrade(buf_socket_t *conn){
    puts("Upgrade");
}

void commit(buf_socket_t *conn){
    puts("Commit");
}

void push(buf_socket_t *conn){
    puts("Push");
}

void create(buf_socket_t *conn){

    // read project
    if (recv_and_verify_project(conn))
        return;
    char *project = conn->data;

    // create local project folder
    mkdir(project, 0755);

    // open local .Manifest file
    char *fname = malloc(strlen(project) + strlen(".Manifest") + 2);
    sprintf(fname, "%s/.Manifest", project);
    int manifest_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // write local .Manifest file
    char manifest_data[] = "0\n";
    int manifest_size = strlen(manifest_data);
    write(manifest_fd, manifest_data, manifest_size);
    close(manifest_fd);

    // send .Manifest file to client
    send_file(fname, conn->sock);

    // cleanup
    free(fname);
}

void destroy(buf_socket_t *conn){
    puts("Destroy");
}

void add(buf_socket_t *conn){
    puts("Add");
}

void remove_cmd(buf_socket_t *conn){
    puts("Remove");
}

void currentversion(buf_socket_t *conn){
    puts("CurrentVersion");
}

void history(buf_socket_t *conn){
    puts("History");
}

void rollback(buf_socket_t *conn){
    puts("Rollback");
}
