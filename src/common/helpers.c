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
 * Send file over socket and wait for ACK.
 */
void send_file(char *filename, int sock, int send_filename){

    // send filename if we should
    send_int(sock, send_filename);
    if (send_filename){
        write(sock, filename, strlen(filename));
        write(sock, "\n", 1);
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
        // TODO: Create directories for file name
        local_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (server_sending_fname){
        // TODO: Create directories for file name
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
 * Sends directory over socket to client by going through a project specfic manifest and sending each file to client
 */
void send_directory(char *dirname, int sock){

    // create a tar file for given directory
    char *create_tar_cmd = malloc(strlen("tar -czvf ") + strlen(dirname) + strlen(".tar.gz ./") + strlen(dirname) + 1);
    sprintf(create_tar_cmd, "tar -czvf %s.tar.gz ./%s", dirname, dirname);
    system(create_tar_cmd);    
    
    // send tar file to client
    char *dir_tar_name = malloc(strlen(dirname) + strlen(".tar.gz") + 1);
    sprintf(dir_tar_name, "%s.tar.gz", dirname);
    send_file(dir_tar_name, sock, 0);

    // clean up (remove tar file)    
    remove(dir_tar_name);
    free(create_tar_cmd);
    free(dir_tar_name);
}

/**
 * Untar a project
 */
void recv_directory(int sock, char *dirname){
    
    // recieve the tar file
    char *dir_tar_name = malloc(strlen(dirname) + strlen(".tar.gz") + 1);
    sprintf(dir_tar_name, "%s.tar.gz", dirname);

    recv_file(sock, dir_tar_name);

    // untar it
    char *untar_cmd = malloc(strlen("tar -xf ") + strlen(dir_tar_name) + 1);
    sprintf(untar_cmd, "tar -xf %s", dir_tar_name);
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
    unsigned long c = getpid();

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
 * Adds a new file to the project's .Manifest if not
 * already there; otherwise marks the file with code "M"
 * for modified.
 *
 * The line is added in the form:
 * <code> <md5_hexdigest> <version> <filename><\n>
 */
void add_to_manifest(char *project, char *filename){

    // open manifest
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);
    init_file_buf(info, manifest);
    
    // open tempfile
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // hash new file to add
    char hexstring[33];
    md5sum(filename, hexstring);

    // recreate manifest
    read_file_until(info, '\n');
    write(fout, info->data, strlen(info->data));
    write(fout, "\n", 1);
    int found_file = 0;
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof){
            break;
        }

        char *code      = malloc(1+1);
        char *version   = malloc(10+1);
        char *hexdigest = malloc(32+1);
        char *fname     = malloc(strlen(info->data)+1);
        sscanf(info->data, "%s %s %s %s", code, hexdigest, version, fname);
        if (!strcmp(filename, fname)){
            found_file = 1;
            write(fout, "M ", 2);
            write(fout, hexdigest, strlen(hexdigest));
            write(fout, " ", 1);
            write(fout, version, strlen(version));
            write(fout, " ", 1);
            write(fout, fname, strlen(fname));
            write(fout, "\n", 1);
        } else{
            write(fout, info->data, strlen(info->data));
            write(fout, "\n", 1);
        }
        free(code);
        free(version);
        free(hexdigest);
        free(fname);
    }

    if (!found_file){
        int buf_size = strlen(hexstring) + strlen(filename) + strlen("A 0  \n ");
        char *data = malloc(buf_size);
        sprintf(data, "A %s 0 %s\n", hexstring, filename);
        write(fout, data, buf_size-1); // no null byte
        free(data);
    }

    // cleanup
    close(fout);
    //rename(tempfile, manifest);
    char *mv_cmd = malloc(strlen("mv ") + strlen(tempfile) + strlen(" ") + strlen(manifest) + 1);
    sprintf(mv_cmd, "mv %s %s", tempfile, manifest);
    system(mv_cmd);
    free(manifest);
    free(mv_cmd);
    clean_file_buf(info);
    remove(tempfile);
}

/**
 * Marks the given file with code "R" in .Manifest.
 * The line is added in the form:
 * <code> <md5_hexdigest> <version> <filename><\n>
 */
void remove_from_manifest(char *project, char *filename){

    // open manifest
    char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
    sprintf(manifest, "%s/.Manifest", project);
    file_buf_t *info = calloc(1, sizeof(file_buf_t));
    init_file_buf(info, manifest);

    // open temp filename - strlen("/tmp/1234567890") = 15
    char tempfile[15+1];
    gen_temp_filename(tempfile);
    int fout = open(tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // recreate manifest
    read_file_until(info, '\n');
    write(fout, info->data, strlen(info->data));
    write(fout, "\n", 1);
    int found_file = 0;
    while (1){
        read_file_until(info, '\n');
        if (info->file_eof){
            break;
        }

        char *code      = malloc(1+1);
        char *version   = malloc(10+1);
        char *hexdigest = malloc(32+1);
        char *fname     = malloc(strlen(info->data)+1);
        sscanf(info->data, "%s %s %s %s", code, hexdigest, version, fname);
        if (!strcmp(filename, fname)){
            found_file = 1;
            write(fout, "R ", 2);
            write(fout, hexdigest, strlen(hexdigest));
            write(fout, " ", 1);
            write(fout, version, strlen(version));
            write(fout, " ", 1);
            write(fout, fname, strlen(fname));
            write(fout, "\n", 1);
        } else{
            write(fout, info->data, strlen(info->data));
            write(fout, "\n", 1);
        }
        free(code);
        free(version);
        free(hexdigest);
        free(fname);
    }

    // cleanup
    close(fout);
    //rename(tempfile, manifest);
    char *mv_cmd = malloc(strlen("mv ") + strlen(tempfile) + strlen(" ") + strlen(manifest) + 1);
    sprintf(mv_cmd, "mv %s %s", tempfile, manifest);
    system(mv_cmd);
    
    free(manifest);
    free(mv_cmd);
    clean_file_buf(info);
    remove(tempfile);

    // error condition
    if (!found_file){
        puts("Did not find file in .Manifest!");
        exit(EXIT_FAILURE);
    }
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
        char *manifest = malloc(strlen(project) + strlen(".Manifest") + strlen("/ "));
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
