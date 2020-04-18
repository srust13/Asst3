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
    sprintf(buf, "%s %s\n", hostname, port);

    // write data to file
    int fd = open(".configure", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    write(fd, buf, buf_len);
    close(fd);
    free(buf);
}

void checkout(char *project){
    puts("Checkout");
    printf("Project: %s\n", project);

    // error case if directory already exists on the client
    struct stat st = {0};
    if (stat(project, &st) != -1){
        puts("Project exist in current directory.");
        exit(EXIT_FAILURE);
    }

    init_socket_server(&sock, "checkout");
    
    // make sure project exists on server
    int proj_exists = server_project_exists(sock, project);
    if (!proj_exists){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    recv_directory(&sock, project);
    close(sock);
}

void update(char *project){
    puts("Update");
    printf("Project: %s\n", project);

    init_socket_server(&sock, "update");
    close(sock);
}

void upgrade(char *project){
    puts("Upgrade");
    printf("Project: %s\n", project);

    init_socket_server(&sock, "upgrade");
    close(sock);
}

void commit(char *project){
    puts("Commit");
    printf("Project: %s\n", project);

    init_socket_server(&sock, "commit");
    close(sock);
}

void push(char *project){
    puts("Push");
    printf("Project: %s\n", project);

    init_socket_server(&sock, "push");
    close(sock);
}

void create(char *project){
    init_socket_server(&sock, "create");

    // make sure project doesn't exist on server
    if (server_project_exists(sock, project)){
        puts("Project already exists on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // create local project folder
    mkdir(project, 0755);

    // receive remote provided .Manifest file
    recv_file(sock, NULL);

    // cleanup
    close(sock);
    puts("Client gracefully disconnected from server");
}

void destroy(char *project){
    puts("Destroy");
    printf("Project: %s\n", project);

    init_socket_server(&sock, "destroy");
    close(sock);
}

void add(char *project, char *filename){
    assert_project_exists_local(project);
    add_to_manifest(project, filename);
}

void remove_cmd(char *project, char *filename){
    assert_project_exists_local(project);
    remove_from_manifest(project, filename);
}

void currentversion(char *project){
    init_socket_server(&sock, "currentversion");

    // make sure project exists on server
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // receive .Manifest into a tempfile
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    recv_file(sock, tempfile);

    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, tempfile);

    // skip first line that just has the name of the project
    puts("\n----------------------------------------------");
    puts("Version | Filename");
    puts("----------------------------------------------");
    read_file_until(info, '\n');
    while (1){
        read_file_until(info, ' ');
        if (info->file_eof)
            break;
        read_file_until(info, ' ');
        read_file_until(info, ' ');
        printf("%s ", info->data);
        read_file_until(info, '\n');
        printf("%s\n", info->data);
    }
    puts("");

    // cleanup
    clean_file_buf(info);
    remove(tempfile);
    puts("Client gracefully disconnected from server");
}

void history(char *project){
    puts("History");
    printf("Project: %s\n", project);

    init_socket_server(&sock, "history");
    close(sock);
}

void rollback(char *project, char *version){
    puts("Rollback");
    printf("Project: %s\n", project);
    printf("Version: %s\n", version);

    init_socket_server(&sock, "rollback");
    close(sock);
}
