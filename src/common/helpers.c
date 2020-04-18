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
void sendline(int sock, char *msg){
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
char *recvline(int sock){
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
 * Send file over socket and wait for ACK.
 */
void send_file(char *filename, int sock){

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
    }
    close(fd);

    // wait for ACK
    wait_for_ack(sock);
}

/**
 * Receives a file from remote and write it
 * locally to the given filename.
 */
void recv_file(int sock, char *dest){

    // open local file
    int local_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);

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
 * Generate random temp file name.
 */
void gen_temp_filename(char *tempfile){
    srand(time(0));
    sprintf(tempfile, "/tmp/");
    int i;
    for(i = 5; i < 15; i++) {
        sprintf(tempfile + i, "%x", rand() % 16);
    }
}

/**
 * Adds a new file to the project's .Manifest
 * The line is added in the form:
 * <version> <md5_hexdigest> <filename><\n>
 */
void add_files_to_manifest(char **filenames, int num_files){
    int fd = open(".Manifest", O_RDWR | O_APPEND);

    int i;
    for (i = 0; i < num_files; i++){
        char *filename = filenames[i];

        // hash file
        char hexstring[33];
        md5sum(filename, hexstring);

        // create buf
        int buf_size = strlen(hexstring) + strlen(filename) + strlen("0  \n ");
        char *data = malloc(buf_size);
        sprintf(data, "0 %s %s\n", hexstring, filename);

        // write "new file" indicator
        write(fd, data, buf_size-1); // no null byte
        free(data);
    }

    // cleanup
    close(fd);
}

/**
 * Removes the given files from .Manifest
 */
void remove_files_from_manifest(char **filenames, int num_files){
    // create random temp filename - strlen("/tmp/1234567890") = 15
    char tempfile[15+1];
    gen_temp_filename(tempfile);

    int fin = open(".Manifest", O_RDONLY);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    char *line = malloc(CHUNK_SIZE);
    int line_len = CHUNK_SIZE;
    int line_idx = 0;
    int eof = 0;
    while (!eof) {
        int bytes_read = 0;
        char *temp = read_file_chunk(fin, &bytes_read, &eof);
        int i;
        for (i = 0; i < bytes_read; i++){
            // double buffer size if full
            if (line_idx == line_len - 1){
                line_len *= 2;
                char *new_buf = realloc(line, line_len);
                if (new_buf == NULL) {
                    puts("Memory allocation failed");
                    free(line);
                    free(temp);
                    exit(EXIT_FAILURE);
                }
                line = new_buf;
            }

            // copy one byte from temp to buf
            line[line_idx++] = temp[i];

            // check if reached newline
            if (line[line_idx-1] == '\n'){
                // check if any of filenames are on this line
                line[line_idx-1] = '\0';
                int j;
                int found = 0;
                for (j = 0; j < num_files; j++){
                    if (strstr(line, filenames[j]) != NULL){
                        found = 1;
                        j = num_files;
                    }
                }

                // write line to new file if not found
                if (!found){
                    write(fout, line, strlen(line));
                    write(fout, "\n", 1);
                }

                // reset line buffer
                line_idx = 0;
            }
        }
        free(temp);
    }
    free(line);

    // cleanup and rename file
    close(fin);
    close(fout);
    rename(tempfile, ".Manifest");
}

/**********************************************************************************
                                  CLIENT HELPERS
***********************************************************************************/

/**
 * Checks if project exists locally by
 * reading project name from .Manifest
 */
void assert_project_exists_local(char *project){
    struct stat st = {0};
    if (stat(".Manifest", &st) == -1){
        puts(".Manifest file does not exist in current directory.");
        exit(EXIT_FAILURE);
    }

    file_buf_t *file_buf = calloc(1, sizeof(file_buf_t));
    file_buf->data = malloc(CHUNK_SIZE);
    file_buf->remaining = malloc(CHUNK_SIZE);
    file_buf->data_buf_size = CHUNK_SIZE;
    file_buf->fd = open(".Manifest", O_RDONLY);

    read_file_until(file_buf, ' ');
    read_file_until(file_buf, '\n');
    close(file_buf->fd);

    int match = !strcmp(file_buf->data, project);
    free(file_buf->remaining);
    free(file_buf->data);
    free(file_buf);

    if (!match){
        puts("Project in .Manifest doesn't match given project name.");
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

    // check if inside a project
    const char *config_file = ".configure";
    if(access(".Manifest", F_OK) != -1){
        config_file = "../.configure";
    }

    // check if file exists
    if(access(config_file, F_OK) == -1){
        puts("Missing .configure file! Did you run configure?");
        exit(EXIT_FAILURE);
    }

    // read data from file
    int fd = open(config_file, O_RDONLY);
    if (fd == -1){
        puts("Can't open .configure file!");
        exit(EXIT_FAILURE);
    }

    char *buf = malloc(CHUNK_SIZE);
    int buf_len = CHUNK_SIZE;
    int buf_idx = 0;
    int eof = 0;

    int reading_hostname = 1;
    char *hostname;
    int port;

    while (!eof) {
        int bytes_read = 0;
        char *temp = read_file_chunk(fd, &bytes_read, &eof);
        int i;
        for (i = 0; i < bytes_read; i++){
            // double buffer size if full
            if (buf_idx == buf_len){
                buf_len *= 2;
                char *new_buf = realloc(buf, buf_len);
                if (new_buf == NULL) {
                    puts("Memory allocation failed");
                    free(buf);
                    free(temp);
                    exit(EXIT_FAILURE);
                }
                buf = new_buf;
            }

            // copy one byte from temp to buf
            buf[buf_idx++] = temp[i];

            // check if reached newline
            if (buf[buf_idx-1] == '\n'){
                if (reading_hostname) {
                    reading_hostname = 0;
                    hostname = malloc(buf_idx);
                    memcpy(hostname, buf, buf_idx-1);
                    hostname[buf_idx-1] = '\0';
                    buf_idx = 0;
                } else {
                    buf[buf_idx-1] = '\0';
                    port = atoi(buf);
                    buf_idx = 0;
                }
            }
        }
        free(temp);
    }
    free(buf);
    close(fd);

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
    sendline(*sock, command);
}

/**
 * Send project name to server and return
 * whether it already exists.
 */
int server_project_exists(int sock, char *project){
    sendline(sock, project);
    return recv_int(sock);
}

/**********************************************************************************
                                  SERVER HELPERS
***********************************************************************************/

/**
 * Receive project name and change into directory.
 * If should_create is set, then it creates the directory
 * if it doesn't exist.
 * Return whether the project exists on server.
 */
int set_create_project(int sock, int should_create){
    char *project = recvline(sock);

    // check if project exists
    struct stat st = {0};
    int exists = (stat(project, &st) == -1) ? 0 : 1;
    send_int(sock, exists);

    // change directory if it exists
    if (exists){
        chdir(project);
    } else if (should_create) {
        mkdir(project, 0755);
        chdir(project);

        // create local .Manifest file
        int manifest_fd = open(".Manifest", O_WRONLY | O_CREAT | O_TRUNC, 0644);

        // write local .Manifest file
        int manifest_size = strlen(project) + strlen("0 \n");
        char *manifest_data = malloc(manifest_size + 1);
        sprintf(manifest_data, "0 %s\n", project);
        write(manifest_fd, manifest_data, manifest_size);
        close(manifest_fd);
        free(manifest_data);
    }
    free(project);
    return exists;
}

/**
 * Receive a filename from the client, then send
 * that file's data over the socket connection.
 */
void send_file_to_client(int sock){
    char *fname = recvline(sock);
    send_file(fname, sock);
    free(fname);
}
