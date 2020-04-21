#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>

#include "helpers.h"

/**********************************************************************************
                                  GENERAL HELPERS
***********************************************************************************/


// ------------------------------------
//        SEND / RECV / ACK
// ------------------------------------

/**
 * Acknowledge receipt of packet.
 */
void ack(int sock){
    write(sock, "ACK", 4);
}

/**
 * Acknowledge receipt of packet after reading socket.
 */
void recv_ack(int sock, void *buf, int num_bytes, int flags){
    recv(sock, buf, num_bytes, flags);
    ack(sock);
}

/**
 * Wait for ACK.
 * Quit if none is received.
 */
void wait_for_ack(int sock){
    char buf[4];
    recv(sock, buf, 4, MSG_WAITALL);
    if (strcmp(buf, "ACK")){
        puts("Failed to receive ACK after writing data");
        printf("Here is the received data:\n");
        int i;
        for (i = 0; i < 4; i++) printf("%02X ", buf[i]);
        close(sock);
        exit(EXIT_FAILURE);
    }
}

/**
 * Write data and wait for acknowledgement.
 */
void send_ack(int sock, void *msg, int msg_len){
    write(sock, msg, msg_len);
    wait_for_ack(sock);
}

/**
 * Send newline-delimited message to server.
 * Wait for an ACK after sending a message.
 */
void send_line(int sock, char *msg){
    write(sock, msg, strlen(msg));
    write(sock, "\n", 1);
    wait_for_ack(sock);
}

/**
 * Receive line from socket and ACK the message.
 * The newline is replaced with a null-byte.
 *
 * The returned pointer must be freed.
 */
char *recv_line(int sock){
    // read chunks from fd until exit condition is reached
    char *line = malloc(CHUNK_SIZE);
    char *temp = malloc(CHUNK_SIZE);
    int line_buf_size = CHUNK_SIZE;
    int data_size = 0;

    while (1){
        int bytes_read = recv(sock, temp, CHUNK_SIZE, 0);

        // ensure that data buffer has enough space
        if (data_size + bytes_read >= line_buf_size){
            line_buf_size *= 2;
            char *new_buf = realloc(line, line_buf_size);
            if (new_buf == NULL) {
                puts("Memory allocation failed");
                free(line);
                close(sock);
                exit(EXIT_FAILURE);
            }
            line = new_buf;
        }

        int i;
        for (i = 0; i < bytes_read; i++) {
            // copy temp to buf one byte at a time
            line[data_size++] = temp[i];

            // check exit condition
            if (line[data_size-1] == '\n'){
                line[data_size-1] = '\0';
                free(temp);
                ack(sock);
                return line;
            }
        }
    }
}

/**
 * Receive an integer from socket.
 */
int recv_int(int sock){
    int number;
    recv_ack(sock, &number, sizeof(number), MSG_WAITALL);
    return ntohl(number);
}

/**
 * Send an integer to socket.
 */
void send_int(int sock, int num){
    int num_to_send = htonl(num);
    send_ack(sock, &num_to_send, sizeof(num_to_send));
}

// ------------------------------------
//               FILES
// ------------------------------------

void write_line(int fd, char *line){
    write(fd, line, strlen(line));
    write(fd, "\n", 1);
}

void move_file(char *src, char *dst){
    char *mv_cmd = malloc(strlen("mv ") + strlen(src) + strlen(" ") + strlen(dst) + 1);
    sprintf(mv_cmd, "mv %s %s", src, dst);
    system(mv_cmd);
    free(mv_cmd);
}

int file_exists_local(char *project, char *fname){
    char *full_fname = malloc(strlen(project) + 1 + strlen(fname) + 1);
    sprintf(full_fname, "%s/%s", project, fname);
    int found = access(full_fname, F_OK) != -1;
    free(full_fname);
    return found;
}

void init_file_buf(file_buf_t *info, char *filename) {
    info->fd = open(filename, O_RDONLY);
    info->data = malloc(CHUNK_SIZE);
    info->remaining = malloc(CHUNK_SIZE);
    info->data_buf_size = CHUNK_SIZE;
}

void clean_file_buf(file_buf_t *info){
    free(info->remaining);
    free(info->data);
    close(info->fd);
    free(info);
}

/**
 * Reads a chunk of data from a file.
 * The returned pointer should be freed after it has been used.
 */
char *read_file_chunk(int fd, int *bytes_read, int *eof){

    // allocate buffer
    char *buf = malloc(CHUNK_SIZE);
    if (buf == NULL){
        puts("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    *bytes_read = 0;
    while (*bytes_read < CHUNK_SIZE){
        // read data from file
        int old_bytes_read = *bytes_read;
        *bytes_read += read(fd, buf + *bytes_read, CHUNK_SIZE - *bytes_read);
        if (*bytes_read == -1){
            puts("Error reading file");
            free(buf);
            exit(EXIT_FAILURE);
        }

        // if bytes_read isn't changing, break out (we've reached EOF)
        if (*bytes_read == old_bytes_read){
            *eof = 1;
            break;
        }
    }
    return buf;
}

/**
 * Read until delim from a file. This file must end in the delim
 * if you decide to reuse this function. It will be replaced with
 * a null-byte, and any bytes remaining in the chunk after the delim
 * are put into "remaining".
 *
 * Note: Since we read from the file in chunks,
 * a previous call to read_until could have read multiple delims
 * of data. The next delim would have been placed into info->remaining,
 * so it's possible we should get the next delim from there instead of
 * actually reading from the file.
 */
void read_file_until(file_buf_t *info, char delim){

    // copy leftover data from "remaining" into "data"
    memcpy(info->data, info->remaining, info->remaining_size);
    int data_size = info->remaining_size;
    info->remaining_size = 0;

    // check if exit condition reached already
    int i;
    for (i = 0; i < data_size; i++){
        if (info->data[i] == delim){
            info->data[i] = '\0';
            info->remaining_size = data_size-i-1;
            memcpy(info->remaining, info->data+i+1, info->remaining_size);
            return;
        }
    }

    // read chunks from file until exit condition is reached
    char *temp = malloc(CHUNK_SIZE);
    while (1){
        int bytes_read = read(info->fd, temp, CHUNK_SIZE);

        // check if nothing left to read
        if (bytes_read == 0){
            info->file_eof = 1;
            free(temp);
            return;
        }

        // ensure that data buffer has enough space
        if (data_size + bytes_read > info->data_buf_size){
            info->data_buf_size *= 2;
            char *new_buf = realloc(info->data, info->data_buf_size);
            if (new_buf == NULL) {
                puts("Memory allocation failed");
                free(temp);
                free(info->data);
                free(info->remaining);
                close(info->fd);
                exit(EXIT_FAILURE);
            }
            info->data = new_buf;
        }

        int i;
        for (i = 0; i < bytes_read; i++) {
            // copy temp to buf one byte at a time
            info->data[data_size++] = temp[i];

            // check exit conditions
            if (info->data[data_size-1] == delim){
                info->data[data_size-1] = '\0';
                info->remaining_size = bytes_read-i-1;
                memcpy(info->remaining, temp+i+1, info->remaining_size);
                free(temp);
                return;
            }
        }
    }
}

/**
 * Recursively make directories for a file path.
 * https://stackoverflow.com/a/9210960/5183816
 */
void mkpath(char* file_path) {
    char *p;
    for (p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, 0755) == -1) {
            if (errno != EEXIST) {
                puts("Error while creating path");
                exit(EXIT_FAILURE);
            }
        }
        *p = '/';
    }
}

/**
 * Send file over socket and wait for ACK.
 */
void send_file(char *filename, int sock, int send_filename){

    // send filename if we should
    send_int(sock, send_filename);
    if (send_filename){
        send_line(sock, filename);
    }

    // send file size
    struct stat st = {0};
    stat(filename, &st);
    send_int(sock, st.st_size);

    // send file data
    int fd = open(filename, O_RDONLY, 0);

    int eof = 0;
    while (!eof){
        int bytes_read = 0;
        char *data = read_file_chunk(fd, &bytes_read, &eof);
        write(sock, data, bytes_read);
        free(data);
    }
    close(fd);

    // wait for ACK
    wait_for_ack(sock);
}

/**
 * Receives a file from remote and write it locally.
 * If the receiver indicated a destination filename,
 * that filename is used. Otherwise, the server must
 * provide a location to write it to.
 */
void recv_file(int sock, char *dest){

    int local_fd;
    char *fname;
    int server_sending_fname = recv_int(sock);
    if (server_sending_fname){
        fname = recv_line(sock);
    }

    if (dest){
        mkpath(dest);
        local_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (server_sending_fname){
        mkpath(fname);
        local_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else {
        puts("The client didn't specify where to save the file,");
        puts("and the server didn't specify a filename!");
        free(fname);
        exit(EXIT_FAILURE);
    }

    if (server_sending_fname){
        free(fname);
    }

    // receive file size
    int file_size = recv_int(sock);

    // receive file bytes, then ACK the file received
    char *data = malloc(CHUNK_SIZE);
    while (file_size > 0){
        int bytes_read = recv(sock, data, CHUNK_SIZE, 0);
        file_size -= bytes_read;
        write(local_fd, data, bytes_read);
    }
    ack(sock);

    // cleanup
    free(data);
    close(local_fd);
}

/**
 * Sends directory over socket to client.
 * Makes use of system tar command with
 * gzip compression.
 */
void send_directory(int sock, char *dirname){

    // create a tar file for given directory
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    char *create_tar_cmd = malloc(
        strlen("tar -czf ") + strlen(tempfile) +
        strlen(".tar.gz ./") + strlen(dirname) + 1
    );
    sprintf(create_tar_cmd, "tar -czf %s.tar.gz ./%s", dirname, dirname);
    system(create_tar_cmd);

    // send tar file to client
    char *dir_tar_name = malloc(strlen(tempfile) + strlen(".tar.gz") + 1);
    sprintf(dir_tar_name, "%s.tar.gz", dirname);
    send_file(dir_tar_name, sock, 0);

    // cleanup
    remove(dir_tar_name);
    free(create_tar_cmd);
    free(dir_tar_name);
}

/**
 * Receive a directory from over the network.
 * Untar and unzip the file received.
 */
void recv_directory(int sock, char *dirname){

    // recieve the tar file
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    char *dir_tar_name = malloc(strlen(tempfile) + strlen(".tar.gz") + 1);
    sprintf(dir_tar_name, "%s.tar.gz", tempfile);
    recv_file(sock, dir_tar_name);

    // untar and unzip it here in repository
    char *untar_cmd = malloc(strlen("tar -xzf ") + strlen(dir_tar_name) + 1);
    sprintf(untar_cmd, "tar -xzf %s", dir_tar_name);
    system(untar_cmd);

    // clean up
    remove(dir_tar_name);
    free(untar_cmd);
    free(dir_tar_name);
}

/**
 * Computes the md5sum of the given file.
 */
void md5sum(char *filename, char *hexstring){
    int n;
    MD5_CTX c;
    char buf[512];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];

    int fd = open(filename, O_RDONLY);

    MD5_Init(&c);
    bytes=read(fd, buf, 512);
    while (bytes > 0){
        MD5_Update(&c, buf, bytes);
        bytes = read(fd, buf, 512);
    }

    MD5_Final(out, &c);
    close(fd);

    // convert bytes to hexstring
    int i;
    for (i = 0; i < 16; i++) sprintf(hexstring+2*i, "%02X", out[i]);
}

/**
 * Seed rand with information from pid and clock/time.
 * https://stackoverflow.com/a/323302/5183816
 */
void seed_rand(){
    unsigned long a = clock();
    unsigned long b = time(NULL);
    unsigned long c = syscall(SYS_gettid);;

    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);

    srand(c);
}

/**
 * Generate random temp file name.
 */
void gen_temp_filename(char *tempfile){
    sprintf(tempfile, "/tmp/");
    int i;
    for(i = 5; i < 15; i++) {
        sprintf(tempfile + i, "%x", rand() % 16);
    }
}

/**
 * Generate random commit file name.
 * Must free pointer recieved from this method.
 */
 char* gen_commit_filename(char *project){
    char *commitfile = malloc(strlen(project) + strlen("/.Commit_") + 10 + 1);
    sprintf(commitfile, "%s/.Commit_", project);
    int startIdx = strlen(commitfile);
    int i;
    for(i = startIdx; i < startIdx + 10; i++) {
        sprintf(commitfile + i, "%x", rand() % 16);
    }
    return commitfile;
}

/**********************************************************************************
                                  CLIENT HELPERS
***********************************************************************************/

/**
 * Checks if project exists in repository.
 */
void assert_project_exists_local(char *project){
    struct stat st = {0};
    if (stat(project, &st) == -1){
        printf("Project does not exist: %s\n", project);
        exit(EXIT_FAILURE);
    }
}

/**
 * Before commands that need the server are run, we first
 * check to see if a .configure file exists. If so, read it
 * and attempt to connect to the server.
 *
 * Then, send a command to the server over the socket.
 */
void init_socket_server(int *sock, char *command){

    // check if file exists
    if(access(".configure", F_OK) == -1){
        puts("Missing .configure file! Did you run configure?");
        exit(EXIT_FAILURE);
    }

    // read data from file
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, ".configure");

    // read hostname
    read_file_until(info, ' ');
    char *hostname = malloc(strlen(info->data)+1);
    strcpy(hostname, info->data);

    // read port
    read_file_until(info, '\n');
    int port = atoi(info->data);
    clean_file_buf(info);

    // create socket
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        puts("Could not create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // convert hostname/ip to binary form of IP address
    if(inet_pton(AF_INET, hostname, &serv_addr.sin_addr) <= 0){
        // try resolving hostname
        struct hostent *he;
        if ((he = gethostbyname(hostname)) == NULL) {
            puts("Could not resolve hostname in .configure file");
            free(hostname);
            exit(EXIT_FAILURE);
        }
        memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    free(hostname);

    // repeatedly try connecting to server
    while (connect(*sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        puts("Connection Failed, retrying in 3 seconds...");
        sleep(3);
    }

    // send command
    puts("Server connected");
    send_line(*sock, command);
}

/**
 * Send project name to server and return
 * whether it already exists.
 */
int server_project_exists(int sock, char *project){
    send_line(sock, project);
    return recv_int(sock);
}

/**********************************************************************************
                                  SERVER HELPERS
***********************************************************************************/

/**
 * Receive and return project name.
 * If should_create is set, then it creates the directory
 * if it doesn't exist.
 *
 * The returned pointer must be freed.
 */
char *set_create_project(int sock, int should_create){
    char *project = recv_line(sock);

    // check if project exists
    struct stat st = {0};
    int exists = (stat(project, &st) == -1) ? 0 : 1;
    send_int(sock, exists);

    // if should_create but it already exists, error
    // if !should_create but it doesn't exist, error
    if ((should_create && exists) || (!should_create && !exists)){
        free(project);
        puts("Create was asked to either create when the project exists,");
        puts("or to not create when the project doesn't exist!");
        return NULL;
    }

    // create directory and Manifest if should create
    if (should_create) {
        mkdir(project, 0755);

        // create local .Manifest file
        char *manifest = malloc(strlen(project) + 1 + strlen(".Manifest") + 1);
        sprintf(manifest, "%s/.Manifest", project);
        int manifest_fd = open(manifest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        free(manifest);

        // write local .Manifest file
        int manifest_size = strlen(project) + strlen("0 \n");
        char *manifest_data = malloc(manifest_size + 1);
        sprintf(manifest_data, "0 %s\n", project);
        write(manifest_fd, manifest_data, manifest_size);
        close(manifest_fd);
        free(manifest_data);
    }
    return project;
}

/**********************************************************************************
                                  MANIFEST HELPERS
***********************************************************************************/

/**
 * Searches for the given filename in the
 * .Manifest file. If found, returns the line.
 *
 * The returned pointer must be freed.
 */
char *search_file_in_manifest(char *manifest, char *search){

    // search for filename in each line of manifest
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, manifest);
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;

        if (strstr(info->data, search)){
            char *line = strdup(info->data);
            clean_file_buf(info);
            return line;
        }
    }
    clean_file_buf(info);
    return NULL;
}

/**
 * Creates a buffer with the given manifest line parameters.
 * The returned pointer must be freed.
 */
char *generate_manifest_line(char code, char *hexdigest, int version, char *fname){
    int out_buf_len = 1 + 1 + 32 + 1 + 10 + 1 + strlen(fname) + 1;
    char *out_buf = malloc(out_buf_len+1);
    sprintf(out_buf, "%c %s %d %s\n", code, hexdigest, version, fname);
    return out_buf;
}

/**
 * Parses a manifest line into components.
 */
manifest_line_t *parse_manifest_line(char *line){
    manifest_line_t *ml = calloc(1, sizeof(manifest_line_t));
    ml->fname = malloc(strlen(line)+1);
    sscanf(line, "%c %s %d %s", &(ml->code), ml->hexdigest, &(ml->version), ml->fname);
    return ml;
}

/**
 * Clean a manifest line's components.
 */
void clean_manifest_line(manifest_line_t *ml){
    free(ml->fname);
    free(ml);
}

/**
 * Adds a new file to the project's .Manifest if not
 * already there. If already previously added, update
 * the hash.
 *
 * The line is added in the form:
 * <code> <md5_hexdigest> <version> <filename><\n>
 */
void add_to_manifest(char *project, char *filename){

    // open manifest
    char *manifest = malloc(strlen(project) + 1 + strlen(".Manifest") + 1);
    sprintf(manifest, "%s/.Manifest", project);
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, manifest);

    // open temp filename - strlen("/tmp/1234567890") = 15
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // recreate manifest first line
    read_file_until(info, '\n');
    write_line(fout, info->data);
    int found_file = 0;

    // line by line copy
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof){
            break;
        }
        manifest_line_t *ml = parse_manifest_line(info->data);

        // different filenames just get copied directly
        if (strcmp(ml->fname, filename)){
            write_line(fout, info->data);
            clean_manifest_line(ml);
            continue;
        }

        // same filename found, some special cases depending on code
        found_file = 1;
        if (ml->code == '-') {
            puts("File already exists in client Manifest; doing nothing");
        } else if (ml->code == 'A'){
            puts("File already marked as 'A', just updating hash in Manifest.");
            md5sum(ml->fname, ml->hexdigest);
        } else if (ml->code == 'D'){
            puts("File already marked as 'D', changing back to '-'");
            ml->code = '-';
        }
        write_line(fout, info->data);
        clean_manifest_line(ml);
    }
    clean_file_buf(info);

    // new filenames get appended with a hash and version 0
    if (!found_file){
        char hexstring[33];
        md5sum(filename, hexstring);
        char *line = generate_manifest_line('A', hexstring, 0, filename);
        write(fout, line, strlen(line));
        free(line);
    }

    // cleanup
    close(fout);
    move_file(tempfile, manifest);
    free(manifest);
}

/**
 * Marks the given file with code "D" in .Manifest.
 * The line is added in the form:
 * <code> <md5_hexdigest> <version> <filename><\n>
 */
void remove_from_manifest(char *project, char *filename){

    // open manifest
    char *manifest = malloc(strlen(project) + 1 + strlen(".Manifest") + 1);
    sprintf(manifest, "%s/.Manifest", project);
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, manifest);

    // open temp filename - strlen("/tmp/1234567890") = 15
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // recreate manifest first line
    read_file_until(info, '\n');
    write_line(fout, info->data);
    int found_file = 0;

    // copy line by line
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof){
            break;
        }
        manifest_line_t *ml = parse_manifest_line(info->data);

        // different filenames just get copied directly
        if (strcmp(ml->fname, filename)){
            write_line(fout, info->data);
            clean_manifest_line(ml);
            continue;
        }

        // same filename found, some special cases depending on code
        found_file = 1;
        if (ml->code == 'D'){
            puts("Already marked for deletion; doing nothing");
            write_line(fout, info->data);
        } else if (ml->code == 'A') {
            puts("Removing file marked as new; totally removing entry from manifest");
        } else if (ml->code == '-') {
            char *line = generate_manifest_line('D', ml->hexdigest, ml->version, ml->fname);
            write(fout, line, strlen(line));
            free(line);
        }
        clean_manifest_line(ml);
    }
    clean_file_buf(info);
    close(fout);
    move_file(tempfile, manifest);
    free(manifest);

    // not an error, since remove "removes" line from manifest anyway,
    // but should at least inform the user
    if (!found_file){
        puts("Did not find file in Manifest");
        puts("Silently completing command");
    }
}

/**
 * Generate a commit file from the project's Manifest file.
 * Returns error if server Manifest file has different version.
 *
 * If a commit file already exists, overwrite it. Commits are
 * a full history of local changes. That haven't been pushed.
 *
 * Returns success
 */
int generate_commit_file(char *commit, char *client_manifest, char *server_manifest){

    // prepare local manifest for reading
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, client_manifest);

    // open temp file for writing new manifest
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // buffer for hash verification
    char cur_hexdigest[32+1];

    // skip first line of manifest
    read_file_until(info, '\n');

    // ======================================
    // How to generate a .Commit file
    // ======================================
    // for each line in client's manifest:
    // 1. verify server line
    //        - if line has code "A", server shouldn't have the filename
    //        - if line has code "D", server SHOULD have the filename
    // 2. if line has code "A" or "D":
    //        - write line to commit
    //        - A should compute a live hash
    // 3.  else if (cur_hash_on_disk != manifest_hash)
    //        - write line to commit with "M", new hash, and incr version

    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;
        manifest_line_t *ml_local = parse_manifest_line(info->data);
        char *server_line = search_file_in_manifest(server_manifest, ml_local->fname);

        char *commit_line = NULL;
        int autogenerated_commit_line = 0;
        int pass_server_check = 1;
        if (ml_local->code == 'D'){
            // commit logs a new deletion
            commit_line = info->data;

            // server SHOULD have this file
            if (!server_line){
                printf("Server manifest does NOT contains file %s"
                      "which is locally marked for deletion!\n", ml_local->fname);
                puts("Client must sync with repository before commiting changes!");
                pass_server_check = 0;
            }
        } else if (ml_local->code == 'A'){
            // commit logs a new addition
            commit_line = info->data;

            // verification correctness
            md5sum(ml_local->fname, cur_hexdigest);

            if (strcmp(ml_local->hexdigest, cur_hexdigest)){
                // hash is not up-to-date
                puts("New file added but Manifest hash is not up-to-date with disk hash.");
                printf("Please first run 'WTF add <project> %s'\n", ml_local->fname);
                pass_server_check = 0;
            } else if (server_line) {
                // server should NOT have this file
                printf("Server manifest DOES contain file %s"
                       "which is locally marked for addition!\n", ml_local->fname);
                puts("Client must sync with repository before commiting changes!");
                pass_server_check = 0;
            }
        } else if (ml_local->code == '-'){
            ml_local->code = 'M';
            // make sure file exists on server
            if (!server_line){
                printf("Server manifest does NOT contains file %s"
                       "which is locally marked as present!\n", ml_local->fname);
                puts("Client must sync with repository before commiting changes!");
                pass_server_check = 0;
            } else {
                // if local file has different hexdigest from server,
                // make sure version is less than server's version
                manifest_line_t *ml_server = parse_manifest_line(server_line);
                if (strcmp(ml_server->hexdigest, ml_local->hexdigest) &&
                            ml_server->version >= ml_local->version){
                    puts("Client must sync with repository before commiting changes!");
                    pass_server_check = 0;
                }
                clean_manifest_line(ml_server);
            }

           // rehash to see if this needs to be added to commit
            md5sum(ml_local->fname, cur_hexdigest);
            if (strcmp(ml_local->hexdigest, cur_hexdigest)){
                // increment version number and change code to 'M'
                commit_line = generate_manifest_line(
                    'M', cur_hexdigest, ml_local->version + 1, ml_local->fname);
                autogenerated_commit_line = 1;
            }
        }
        free(server_line);

        if (!pass_server_check){
            close(fout);
            clean_file_buf(info);
            clean_manifest_line(ml_local);
            remove(tempfile);
            return 0;
        }

        // write results to commit file appropriately
        if (commit_line){
            if (autogenerated_commit_line){
                write(fout, commit_line, strlen(commit_line));
                free(commit_line);
            } else {
                write_line(fout, commit_line);
            }
            printf("%c %s\n", ml_local->code, ml_local->fname);
        }
        clean_manifest_line(ml_local);
    }
    close(fout);
    clean_file_buf(info);
    move_file(tempfile, commit);
    return 1;
}

/**
 * Generates a tar file with files that have code "A" or "M" in the .Commit.
 * Returns the name of the tar file. Should be freed after use.
 */
char *generate_am_tar(char *commitPath) {

    // maintain list of files to tar
    int file_count = 0;
    int max_file_count = 50;
    char **files_to_tar = malloc(max_file_count * sizeof(char *));
    int cmd_length = 0;

    // get list of files to tar from commit file's "A" or "M" codes
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, commitPath);

    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;
        manifest_line_t *ml = parse_manifest_line(info->data);

        // add "A"/"M" files to tar list
        if (ml->code == 'A' || ml->code == 'M'){
            if (file_count >= max_file_count){
                max_file_count *= 2;
                files_to_tar = realloc(files_to_tar, max_file_count * sizeof(char *));
            }
            files_to_tar[file_count++] = strdup(ml->fname);
            cmd_length += strlen(ml->fname) + 1; // name and space
        }
        clean_manifest_line(ml);
    }

    // create tar
    char *tar_name = malloc(15 + strlen(".tar.gz") + 1);
    gen_temp_filename(tar_name);
    sprintf(tar_name, "%s.tar.gz", tar_name);

    char *cmd = malloc(strlen("tar czf ") + strlen(tar_name) + cmd_length);
    sprintf(cmd, "tar czf %s", tar_name);
    char *cur = cmd + strlen(cmd);
    int i;
    for (i = 0; i < file_count; i++){
        *cur = ' ';
        cur += 1;
        strcpy(cur, files_to_tar[i]);
        cur += strlen(files_to_tar[i]);
        free(files_to_tar[i]);
    }
    system(cmd);

    // cleanup
    free(cmd);
    free(files_to_tar);
    clean_file_buf(info);
    return tar_name;
}

/**
 * Goes through commit file lines to get list of
 * only "modified" lines. The modified lines form
 * a linked list.
 *
 * Each node in the list must get passed to clean_manifest_line.
 */
manifest_line_t *modified_files_from_commit(char *commit){

    manifest_line_t *head = NULL;
    manifest_line_t *cur = NULL;

    // read from commit line by line
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, commit);
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;
        manifest_line_t *ml = parse_manifest_line(info->data);
        if (ml->code == 'M'){
            if (!cur){
                cur = ml;
                head = cur;
            } else {
                cur->next = ml;
                cur = cur->next;
            }
        } else {
            clean_manifest_line(ml);
        }
    }
    clean_file_buf(info);
    return head;
}

/**
 * After pushing, recreate the Manifest file with
 * <code> = "-" on client to files.
 *
 * The line is added in the form:
 * <code> <md5_hexdigest> <version> <filename><\n>
 */
void regenerate_manifest(char *client_manifest, char *commit){

    // get list of all "modified" files from commit
    manifest_line_t *mod_lines = modified_files_from_commit(commit);

    // read from client manifest
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, client_manifest);

    // open tempfile to write new manifest to
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // write the incremented project version to the temp file
    read_file_until(info, ' ');
    char newVersion_buf[10];
    sprintf(newVersion_buf, "%d", atoi(info->data) + 1);
    write(fout, newVersion_buf, strlen(newVersion_buf));
    write(fout, " ", 1);

    // write the project name
    read_file_until(info, '\n');
    write_line(fout, info->data);

    // go line by line in manifest
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;
        manifest_line_t *ml = parse_manifest_line(info->data);

        // skip deleted lines
        if (ml->code == 'D'){
            clean_manifest_line(ml);
            continue;
        }

        // if this line appears in "modified" list,
        // copy new hash and version num
        if (ml->code == '-'){
            manifest_line_t *cur = mod_lines;
            while (cur){
                if (!strcmp(ml->fname, cur->fname)){
                    strcpy(ml->hexdigest, cur->hexdigest);
                    ml->version = cur->version;
                    break;
                }
                cur = cur->next;
            }
        }

        // write new line to tempfile
        char *newline = generate_manifest_line('-', ml->hexdigest, ml->version, ml->fname);
        write(fout, newline, strlen(newline));
        free(newline);
        clean_manifest_line(ml);
    }

    while (mod_lines){
        manifest_line_t *next = mod_lines->next;
        clean_manifest_line(mod_lines);
        mod_lines = next;
    }

    close(fout);
    clean_file_buf(info);
    move_file(tempfile, client_manifest);
}

/**********************************************************************************
                                  COMMIT HELPERS
***********************************************************************************/

/**
 * For every .Commit file in the project dir, compute the hash of it 
 * If it's equal to the client commit hash, return name of that file
 * If the commit file wasn't found, return null byte
 */
char* commit_exists(char *project, char *client_hex){

    // open the project directory 
    struct dirent *de;  
    DIR *proj_dir = opendir(project);   
    char *commitBuff = calloc(8, sizeof(char));

    // go through the files of the project directory
    while ((de = readdir(proj_dir)) != NULL) {
        // only read files with names longer than ".Commit" length
        if (strlen(de->d_name) >= 7) {
            int i;
            for (i = 0; i < 7; i++) {
                commitBuff[i] = (de->d_name)[i];
            }

            // if the file name starts with .Commit, check its hash is the same as client .Commit
            if (!strcmp(commitBuff, ".Commit")) {
                char hexstring[33];
                md5sum(de->d_name, hexstring);

                // if hash is same, return file name
                if (!strcmp(hexstring, client_hex)) {
                    closedir(proj_dir);   
                    free(commitBuff);  
                    char *commit_name = malloc(strlen(de->d_name) + 1 );
                    sprintf(commit_name, "%s", de_d_name);
                    return commit_name;
                }                
            }
        }
    }   
    closedir(proj_dir);   
    free(commitBuff);  
    return "\0";
}

/**
 * Remove all .Commit files from given project
 */
void removeAllCommits(char *project) {

    // open the project directory 
    struct dirent *de;  
    DIR *proj_dir = opendir(project);   
    char *commitBuff = calloc(8, sizeof(char));
    int commitFile_length = 18;

    char *rm_commit_path = malloc(strlen(project) + strlen("/") + commitFile_length + 1);
    
    // go through the files of the project directory
    while ((de = readdir(proj_dir)) != NULL) {
        // only read files with names longer than ".Commit" length
        if (strlen(de->d_name) >= 7) {
            int i;
            for (i = 0; i < 7; i++) {
                commitBuff[i] = (de->d_name)[i];
            }

            // delete all .Commit files 
            if (!strcmp(commitBuff, ".Commit")) {
                sprintf(rm_commit_path, "%s/%s", project, de->d_name);
                remove(rm_commit_path);
            }
        }
    }   
    closedir(proj_dir);   
    free(commitBuff); 
    free(rm_commit_path); 
    return;
}

void removeAll_dFiles(char *commit) {
    // read from commit file
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, commit);

    while (1){
        read_file_until(info, '\n');
        if (info->file_eof)
            break;
        manifest_line_t *ml = parse_manifest_line(info->data);

        // remove all "D" files
        if (ml->code == 'D'){
            remove(ml->fname);
        }
        clean_manifest_line(ml);
    }
    clean_file_buf(info);
}
