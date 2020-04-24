#define _GNU_SOURCE
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
    char *buf;
    asprintf(&buf, "%s %s\n", hostname, port);

    // write data to file
    int fd = open(".configure", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    write(fd, buf, strlen(buf));
    close(fd);
    free(buf);
}

void checkout(char *project){

    // check if project already exists locally
    struct stat st = {0};
    if (stat(project, &st) != -1){
        puts("Project already exists in local repository.");
        exit(EXIT_FAILURE);
    }

    // check if project exists on server
    init_socket_server(&sock, "checkout");
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    recv_directory(sock, project);
    close(sock);
}

void update(char *project){
    // connect to server and make sure project exists
    init_socket_server(&sock, "update");
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char *conflict;
    asprintf(&conflict, "%s/.Conflict", project);

    // retrieve server .Manifest and parse version
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    recv_file(sock, tempfile);
    int server_manifest_version = get_manifest_version(tempfile);

    // retrieve client .Manifest and parse version
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    int client_manifest_version = get_manifest_version(manifest);

    // if client and server .Manifest versions are same: write blank .Update file and remove .Conflict
    if (server_manifest_version == client_manifest_version){
        puts("Client and server .Manifest versions match!");
        puts("Up to date.");

        char *update;
        asprintf(&update, "%s/.Update", project);
        int fout = open(update, O_RDONLY | O_CREAT | O_TRUNC, 0644);

        free(update);
        close(fout);
    } else {
        // handle partial success and failure cases... create a .Conflict (if necessary) and .Update
        generate_update_conflict_files(project, manifest, tempfile);
    }

    // remove .Conflict file if empty
    struct stat st = {0};
    stat(conflict, &st);
    if (st.st_size == 0){
        remove(conflict);
    }

    free(conflict);
    remove(tempfile);
    free(manifest);
    close(sock);
}

void upgrade(char *project){

    // check for valid .Update and no .Conflict
    char *update;
    asprintf(&update, "%s/.Update", project);
    struct stat st_update = {0};
    int update_exists = stat(update, &st_update) != -1;

    char *conflict;
    asprintf(&conflict, "%s/.Conflict", project);
    struct stat st_conflict = {0};
    int conflict_exists = stat(conflict, &st_conflict) != -1;

    if (!update_exists) {
        puts("No update file; please run Update first.");
        puts("Client disconnecting.");
        exit(EXIT_FAILURE);
    } else if (conflict_exists) {
        puts("Resolve all conflicts before updating.");
        puts("Client disconnecting.");
        exit(EXIT_FAILURE);
    } else if (st_update.st_size == 0) {
        puts("Up to date.");
        free(update);
        free(conflict);
        close(sock);
        return;
    }

    // connect to server and make sure project exists
    init_socket_server(&sock, "upgrade");
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);

    // retrieve server .Manifest version
    int server_manifest_version = recv_int(sock);

    // remove all files from .Manifest marked "D" in update
    remove_dFiles_from_manifest(manifest, update, server_manifest_version);

    // send .Update file to server
    send_file(update, sock, 0);

    // recieve tar of modified and added files from server
    char tar[15+1];
    gen_temp_filename(tar);
    recv_file(sock, tar);

    // if tar is not empty, untar it into project
    struct stat st_tar = {0};
    stat(tar, &st_tar);
    if(st_tar.st_size > 0){
        char *untar_cmd = malloc(strlen("tar xzf ") + strlen(tar) + 1);
        sprintf(untar_cmd, "tar xzf %s", tar);
        system(untar_cmd);
        free(untar_cmd);
        remove(tar);
    }

    // after pulling in all changes, recreate the manifest to account for modified/added files
    regenerate_manifest_from_update(manifest, update);
    free(manifest);

    //remove(update); TODO: remove this as a comment
    free(update);
    free(conflict);
    close(sock);
}

void commit(char *project){

    // check if client already has non-empty .Update file
    char *update;
    asprintf(&update, "%s/.Update", project);
    struct stat st = {0};
    if (stat(update, &st) && st.st_size > 0){
        puts("Project already has a .Update file!");
        exit(EXIT_FAILURE);
    }
    free(update);

    // check if client already has a .Conflict file
    if (file_exists_local(project, ".Conflict")){
        puts("Project has a .Conflict that must be resolved first!");
        exit(EXIT_FAILURE);
    }

    // connect to server and make sure project exists
    init_socket_server(&sock, "commit");
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // retrieve server .Manifest and parse version
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    recv_file(sock, tempfile);
    int server_manifest_version = get_manifest_version(tempfile);

    // retrieve client .Manifest and parse version
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    int client_manifest_version = get_manifest_version(manifest);

    // verify .Manifest versions are equal
    if (server_manifest_version != client_manifest_version){
        puts("Client and server .Manifest versions don't match!");
        puts("You need to update to the latest server code first.");
        send_int(sock, 0);
        free(manifest);
        remove(tempfile);
        close(sock);
        exit(EXIT_FAILURE);
    }

    // create .Commit file
    char *commit;
    asprintf(&commit, "%s/.Commit", project);
    if (!generate_commit_file(commit, manifest, tempfile)){
        send_int(sock, 0);
        remove(commit);
        close(sock);
        free(manifest);
        free(commit);
        remove(tempfile);
        exit(EXIT_FAILURE);
    }

    // send success
    send_int(sock, 1);

    // send commit file to server
    send_file(commit, sock, 1);

    // cleanup
    free(manifest);
    free(commit);
    remove(tempfile);
    close(sock);
}

void push(char *project){

    // check if project exists locally
    struct stat st = {0};
    if (stat(project, &st) == -1){
        puts("Project doesn't exist in local repository.");
        exit(EXIT_FAILURE);
    }

    // check if client has a .Commit file
    if (!file_exists_local(project, ".Commit")){
        puts("Project has no .Commit to push!");
        exit(EXIT_FAILURE);
    }

    // check if project exists on server
    init_socket_server(&sock, "push");
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // send local .Commit file md5sum to server and wait for acceptance
    char *commitPath;
    asprintf(&commitPath, "%s/.Commit", project);
    char digest[32+1];
    md5sum(commitPath, digest);
    send_line(sock, digest);

    int success = recv_int(sock);

    if (success){
        // generate a tar of all A/M files in .Commit and send to server
        char *tar_name = generate_am_tar(commitPath);
        send_file(tar_name, sock, 0);

        // regenerate manifest file from .Commit
        char *manifestPath;
        asprintf(&manifestPath, "%s/.Manifest", project);
        regenerate_manifest(manifestPath, commitPath);

        // send the manifest to the server
        send_file(manifestPath, sock, 0);

        remove(tar_name);
        free(tar_name);
        free(manifestPath);
    } else {
        puts("Client push rejected");
    }

    // cleanup
    remove(commitPath);
    free(commitPath);
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
    init_socket_server(&sock, "destroy");
    if (!server_project_exists(sock, project)){
        puts("Project does not exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }
    close(sock);
    puts("Client gracefully disconnected from server");
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

    file_buf_t *info = init_file_buf(tempfile);

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
    init_socket_server(&sock, "rollback");
    if (!server_project_exists(sock, project)){
        puts("Project doesn't exist on server!");
        puts("Client disconnecting.");
        close(sock);
        exit(EXIT_FAILURE);
    }
    send_line(sock, version);
    if (!recv_int(sock)){
        puts("Version not found on server!");
        exit(EXIT_FAILURE);
    }
    close(sock);
}
