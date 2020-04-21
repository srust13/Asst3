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


// Confused on why directions say to send .Commit file to server and then check if server has the .Commit file? 
// Just send hash from client and check if the .Commit file is already on server from Commiting process?
void push(int sock, char *project){
    
    // receive client's .Commit
    char *commit_path = gen_commit_filename(project);
    //recv_file(sock, commit_path); // TODO: Delete this since we're using md5sum
    char *client_commit_hash = recv_line(sock);

    // find out if a .Commit file md5sum in the current project matches .Commit md5sum recieved from client    
    char *commitMatch = commit_exists(project, client_commit_hash);

    // if received commit has expired; inform client and close connection
    if(!strcmp("\0", commitMatch)) {
        send_int(sock, 0);
        remove(commit_path);
        return;
    }

    send_int(sock, 1);

    // Expire all .Commit files for this project
    removeAllCommits(project);

    // receive tar with changed files
    char temp_tar[15+1];
    gen_temp_filename(temp_tar);
    recv_file(sock, temp_tar);  // TODO: Should file extension be .tar to untar or unnecessary? 

    // untar files into project directory
    char *untar_cmd = malloc(strlen("tar xzf ") + strlen(temp_tar) + strlen(" -C /") + strlen(project) + 1);
    sprintf(untar_cmd, "tar xzf %s -C /%s", temp_tar, project);
    system(untar_cmd);
    free(untar_cmd);

    // remove current .Manifest  
    char *manifestPath = malloc(strlen(project) + strlen("/.Manifest") + 1);
    sprintf(manifestPath, "%s/.Manifest", project);
    remove(manifestPath);

    // replace old .Manifest with new updated .Manifest from client
    recv_file(sock, manifestPath);    

    // cleanup
    free(commit_path);
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
