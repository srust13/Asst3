#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "helpers.h"

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
    read_chunks(conn, '\n', 0);
    char *project = conn->data;

    // check if project exists
    int success = 0;
    struct stat st = {0};
    if (stat(project, &st) != -1){
        write(conn->sock, &success, sizeof(success));
        return;
    } else {
        success = 1;
        write(conn->sock, &success, sizeof(success));
    }

    // create local project folder
    mkdir(project, 0755);

    // open local .Manifest file
    char *fname = malloc(strlen(project) + strlen(".Manifest") + 2);
    sprintf(fname, "%s/.Manifest", project);

    int manifest_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(fname);

    // generate .Manifest file and send data to client
    char manifest_data[] = "version: 0\n";
    int manifest_size = strlen(manifest_data);
    write(manifest_fd, manifest_data, manifest_size);

    // send size of file and file contents over socket
    int manifest_size_network = htonl(manifest_size);
    write(conn->sock, &manifest_size_network, sizeof(manifest_size_network));
    write(conn->sock, manifest_data, manifest_size);

    // cleanup
    close(manifest_fd);
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
