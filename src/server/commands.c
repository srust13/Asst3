#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/helpers.h"

void checkout(int sock, char *project){
    send_directory(sock, project);
}

void update(int sock, char *project){
    puts("Update");
}

void upgrade(int sock, char *project){
    puts("Upgrade");
}

void commit(int sock, char *project){
    // send manifest
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + 1);
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);

    // receive client success msg on creating .Commit
    if (!recv_int(sock)){
        puts("Client encountered error when trying to .Commit");
        return;
    }

    // receive client .Commit file
    recv_file(sock, NULL);
    puts("Received new .Commit file");
}

void push(int sock, char *project){    
    // receive client's .Commit ms5sum
    char *client_commit_hash = recv_line(sock);

    // find out if a .Commit file md5sum in the current project matches .Commit md5sum recieved from client    
    char *commitMatch = commit_exists(project, client_commit_hash);

    // if received commit has expired; inform client and close connection
    if(!commitMatch) {
        send_int(sock, 0);
        free(client_commit_hash);
        return;
    }

    send_int(sock, 1);

    // remove all files that have been deleted in the .Commit
    removeAll_dFiles(commitMatch);

    // Expire all .Commit files for this project
    removeAllCommits(project);

    // receive tar with changed files
    char temp_tar[15+1];
    gen_temp_filename(temp_tar);
    recv_file(sock, temp_tar);  

    // untar files into project directory
    char *untar_cmd = malloc(strlen("tar xzf ") + strlen(temp_tar) + 1);
    sprintf(untar_cmd, "tar xzf %s", temp_tar);
    system(untar_cmd);
    free(untar_cmd);

    // get manifest path
    char *manifestPath = malloc(strlen(project) + strlen("/.Manifest") + 1);
    sprintf(manifestPath, "%s/.Manifest", project);
    
    // replace old .Manifest with new updated .Manifest from client
    recv_file(sock, manifestPath);    

    // cleanup
    free(commitMatch);
    free(client_commit_hash);
    free(manifestPath);
    remove(temp_tar);
}

void create(int sock, char *project){
    // send requested .Manifest to client
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 1);
    free(manifest);
}

void destroy(int sock, char *project){
    char *cmd = malloc(strlen("rm -rf ") + strlen(project) + 1);
    sprintf(cmd, "rm -rf %s", project);
    system(cmd);
    free(cmd);
}

void currentversion(int sock, char *project){
    // send requested .Manifest to client
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);
}

void history(int sock, char *project){
    puts("History");
}

void rollback(int sock, char *project){
    puts("Rollback");
}
