#define _GNU_SOURCE
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
    // send manifest
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);
}

void upgrade(int sock, char *project){

    // recieve client .Update
    char update[15+1];
    gen_temp_filename(update);
    recv_file(sock, update);

    // generate tar of added/modified files and send to client
    char *tar = generate_am_tar(update);
    send_file(tar, sock, 0);
    remove(tar);
    remove(update);
    free(tar);

    // send manifest version to client
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    send_int(sock, get_manifest_version(manifest));
    free(manifest);
}

void commit(int sock, char *project){
    // send manifest
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);

    // receive client success msg on creating .Commit
    if (!recv_int(sock)){
        puts("Client encountered error when trying to .Commit");
        return;
    }

    // receive client .Commit file
    char *commit_path = gen_commit_filename(project);
    recv_file(sock, commit_path);
    free(commit_path);
    puts("Received new .Commit file");
}

void push(int sock, char *project){

    // find out if a .Commit file md5sum in the current project matches
    // .Commit md5sum recieved from client
    char *client_commit_hash = recv_line(sock);
    char *commitMatch = commit_exists(project, client_commit_hash);
    free(client_commit_hash);

    // if received commit has expired; inform client and close connection
    if(!commitMatch) {
        send_int(sock, 0);
        return;
    }
    send_int(sock, 1);

    // receive tar with changed files
    char temp_tar[15+1];
    gen_temp_filename(temp_tar);
    recv_file(sock, temp_tar);

    // untar files into project directory if we have non-empty tar
    struct stat st = {0};
    stat(temp_tar, &st);
    if(st.st_size > 0){
        char *untar_cmd;
        asprintf(&untar_cmd, "tar xzf %s", temp_tar);
        system(untar_cmd);
        free(untar_cmd);
    }

    // replace old .Manifest with new updated .Manifest from client
    char *manifestPath;
    asprintf(&manifestPath, "%s/.Manifest", project);
    recv_file(sock, manifestPath);

    // backup A/M files and remove D files
    int version = get_manifest_version(manifestPath);
    update_repo_from_commit(commitMatch, project, version);
    free(commitMatch);

    // Expire all .Commit files for this project
    remove_all_commits(project);

    // backup .Manifest
    char *cmd;
    asprintf(&cmd, "tar -czf backups/%s_%d %s", manifestPath, version, manifestPath);
    system(cmd);
    free(cmd);

    // cleanup
    free(manifestPath);
    remove(temp_tar);
}

void create(int sock, char *project){
    // backup manifest
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);

    char *backup_manifest;
    asprintf(&backup_manifest, "backups/%s", manifest);
    mkpath(backup_manifest);
    free(backup_manifest);

    char *cmd;
    asprintf(&cmd, "tar -czf backups/%s_%d %s", manifest, 0, manifest);
    system(cmd);
    free(cmd);

    // send requested .Manifest to client
    send_file(manifest, sock, 1);
    free(manifest);
}

void destroy(int sock, char *project){
    char *cmd;
    asprintf(&cmd, "rm -rf %s", project);
    system(cmd);
    free(cmd);
}

void currentversion(int sock, char *project){
    // send requested .Manifest to client
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);
}

void history(int sock, char *project){
    char *manifest;
    asprintf(&manifest, "%s/.Manifest", project);
    int version = get_manifest_version(manifest);
    free(manifest);

    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    struct stat st = {0};
    int i;
    for (i = 1; i >= 0; i++){
        char *backup;
        asprintf(&backup, "history/%s/.Commit_%d", project, i);
        if (stat(backup, &st) != -1){

            // write version number
            char *num;
            asprintf(&num, "%d\n", i);
            write(fout, num, strlen(num));
            free(num);

            // write file contents
            int eof = 0;
            int bytes_read = 0;
            int fin = open(backup, O_RDONLY);
            while (!eof){
                char *buf = read_file_chunk(fin, &bytes_read, &eof);
                write(fout, buf, bytes_read);
            }
            close(fin);

        } else
            i = -10; // done
        free(backup);
    }
    close(fout);
    send_file(tempfile, sock, 0);
    remove(tempfile);
}

void rollback(int sock, char *project){
    char *version = recv_line(sock);

    // check if project version exists
    char *manifest_backup;
    asprintf(&manifest_backup, "backups/%s/.Manifest_%s", project, version);
    struct stat st = {0};
    int exists = stat(manifest_backup, &st) != -1;
    if (!exists){
        free(version);
        free(manifest_backup);
        send_int(sock, exists);
        return;
    }
    free(manifest_backup);

    // delete old .Commit files in history
    int i;
    for (i = atoi(version) + 1; i >= 0; i++){
        char *backup;
        asprintf(&backup, "history/%s/.Commit_%d", project, i);
        if (stat(backup, &st) != -1){
            remove(backup);
        } else
            i = -10; // done
        free(backup);
    }

    // execute rollback
    rollback_every_file(project, version);
    free(version);
    send_int(sock, exists);
}
