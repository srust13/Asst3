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
    char *manifest = malloc(strlen(project) + strlen("/.Manifest") + 1);
    sprintf(manifest, "%s/.Manifest", project);
    send_file(manifest, sock, 0);
    free(manifest);
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
        char *untar_cmd = malloc(strlen("tar xzf ") + strlen(temp_tar) + 1);
        sprintf(untar_cmd, "tar xzf %s", temp_tar);
        system(untar_cmd);
        free(untar_cmd);
    }

    // replace old .Manifest with new updated .Manifest from client
    char *manifestPath = malloc(strlen(project) + strlen("/.Manifest") + 1);
    sprintf(manifestPath, "%s/.Manifest", project);
    recv_file(sock, manifestPath);

    // backup A/M files and remove D files
    update_repo_from_commit(commitMatch);
    free(commitMatch);

    // Expire all .Commit files for this project
    remove_all_commits(project);

    // backup .Manifest
    int version = get_manifest_version(manifestPath);
    char *cmd = malloc(strlen("tar -czf ") + strlen(manifestPath) + strlen("backups/") + strlen(manifestPath) + 1 + 10 + 1);
    sprintf(cmd, "tar -czf backups/%s_%d %s", manifestPath, version, manifestPath);
    system(cmd);
    free(cmd);

    // cleanup
    free(manifestPath);
    remove(temp_tar);
}

void create(int sock, char *project){
    // backup manifest
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);

    char *backup_manifest = malloc(strlen("backups/") + strlen(manifest) + 1);
    sprintf(backup_manifest, "backups/%s", manifest);
    mkpath(backup_manifest);
    free(backup_manifest);

    char *cmd = malloc(strlen("tar -czf ") + strlen(manifest) + strlen("backups/") + strlen(manifest) + 1 + 10 + 1);
    sprintf(cmd, "tar -czf backups/%s_%d %s", manifest, 0, manifest);
    system(cmd);
    free(cmd);

    // send requested .Manifest to client
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
    char *version = recv_line(sock);

    // check if project version exists
    char *manifest_backup = malloc(
        strlen("backups/") + strlen(project) + strlen("/.Manifest_") + strlen(version) + 1);
    sprintf(manifest_backup, "backups/%s/.Manifest_%s", project, version);
    struct stat st = {0};
    int exists = stat(manifest_backup, &st) != -1;
    if (!exists){
        send_int(sock, exists);
        return;
    }
    free(manifest_backup);

    // create temp dir for extraction
    char tempdir[15+1];
    gen_temp_filename(tempdir);
    mkdir(tempdir, 0755);

    // extract manifest to tempdir
    char *manifest = malloc(strlen(tempdir) + 1 + strlen(project) + strlen("/.Manifest") + 1);
    sprintf(manifest, "%s/.Manifest", project);
    rollback_file(manifest, atoi(version), tempdir);
    sprintf(manifest, "%s/%s/.Manifest", tempdir, project);

    // open and read manifest line-by-line
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, manifest);
    read_file_until(info, '\n');

    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;
        manifest_line_t *ml = parse_manifest_line(info->data);
        rollback_file(ml->fname, ml->version, tempdir);
        clean_manifest_line(ml);
    }
    clean_file_buf(info);
    free(version);

    // move tempdir to realdir
    char *rm_old = malloc(strlen("rm -rf ") + strlen(project) + 1);
    sprintf(rm_old, "rm -rf %s", project);
    system(rm_old);
    free(rm_old);

    char *mv_new = malloc(strlen("mv ") + strlen(tempdir) + 1 + strlen(project) + 1);
    sprintf(mv_new, "mv %s/%s .", tempdir, project);
    system(mv_new);
    free(mv_new);

    rmdir(tempdir);
    send_int(sock, exists);
}
