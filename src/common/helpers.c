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

#include "helpers.h"

/**********************************************************************************
                                  GENERAL HELPERS
***********************************************************************************/

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
void sendline_ack(int sock, char *msg){
    write(sock, msg, strlen(msg));
    write(sock, "\n", 1);
    wait_for_ack(sock);
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
 * Send file
 */
void send_file(char *filename, int sock){

    // send file size
    struct stat st = {0};
    stat(filename, &st);
    int file_size = st.st_size;
    int file_size_nl = htonl(file_size);
    send_ack(sock, &file_size_nl, sizeof(file_size_nl));

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
 * Receives a file from remote and
 * write it locally to the given filename.
 */
void recv_file(int sock, char *fname){

    // open local file
    int manifest_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    // receive file size
    int file_size = 0;
    recv_ack(sock, &file_size, sizeof(file_size), MSG_WAITALL);
    file_size = ntohl(file_size);

    // receive file bytes, then ACK the file received
    char *data = malloc(CHUNK_SIZE);
    while (file_size > 0){
        int bytes_read = recv(sock, data, CHUNK_SIZE, 0);
        file_size -= bytes_read;
        write(manifest_fd, data, bytes_read);
    }
    ack(sock);

    // cleanup
    free(data);
    close(manifest_fd);
}


/**********************************************************************************
                                  CLIENT HELPERS
***********************************************************************************/

/**
 * Before any command besides "configure" is run, we first
 * check to see if a .configure file exists. If so, read it
 * and attempt to connect to the server.
 */
void set_socket(int *sock){

    // check if file exists
    if(access(".configure", F_OK) == -1){
        puts("Missing .configure file! Did you run configure?");
        exit(EXIT_FAILURE);
    }

    // read data from file
    int fd = open(".configure", O_RDONLY);
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

    puts("Server connected");
}

/**
 * Sends project name to server and returns
 * whether it already exists.
 */
int server_project_exists(int sock, char *project){
    sendline_ack(sock, project);

    int success = 0;
    recv_ack(sock, &success, sizeof(success), MSG_WAITALL);
    return ntohl(success);
}

/**********************************************************************************
                                  SERVER HELPERS
***********************************************************************************/

/**
 * Receive project name and set it in conn->data.
 * Return whether the project exists on server.
 */
int recv_and_verify_project(buf_socket_t *conn){
    read_app_data_from_socket(conn, '\n', 0);

    // check if project exists
    struct stat st = {0};
    int exists = (stat(conn->data, &st) == -1) ? htonl(0) : htonl(1);
    send_ack(conn->sock, &exists, sizeof(exists));
    return exists;
}

/**
 * Reads chunks from socket until num_bytes has been read, or if
 * num_bytes is 0, then until delimiter is read. Any bytes left
 * in the chunk after the exit condition has been reached are
 * placed in conn->remaining. Delimiters are replaced with a
 * null byte. Finally, sends an ACK to indicate we have received
 * all data from the client.
 *
 * Note:
 * It is possible that this function doesn't read from
 * the socket at all. This occurs if there is existing data in
 * the "remaining" already satisfies the exit condition.
 */
void read_app_data_from_socket(buf_socket_t *conn, char delim, int num_bytes){

    // copy leftover data from "remaining" into "data"
    memcpy(conn->data, conn->remaining, conn->remaining_size);
    int data_size = conn->remaining_size;
    conn->remaining_size = 0;

    // check if exit condition reached already
    if (num_bytes != 0){
        if (data_size >= num_bytes){
            conn->remaining_size = data_size - num_bytes;
            memcpy(conn->remaining, conn->data + num_bytes, conn->remaining_size);
            ack(conn->sock);
            return;
        }
    } else {
        int i;
        for (i = 0; i < data_size; i++){
            if (conn->data[i] == delim){
                conn->data[i] = '\0';
                conn->remaining_size = data_size - i - 1;
                memcpy(conn->remaining, conn->data + i + 1, conn->remaining_size);
                ack(conn->sock);
                return;
            }
        }
    }

    // read chunks from socket until exit condition is reached
    char *temp = malloc(CHUNK_SIZE);
    while (1){
        int bytes_read = recv(conn->sock, temp, CHUNK_SIZE, 0);
        // ensure that data buffer has enough space
        if (data_size + bytes_read > conn->data_buf_size){
            conn->data_buf_size *= 2;
            char *new_buf = realloc(conn->data, conn->data_buf_size);
            if (new_buf == NULL) {
                puts("Memory allocation failed");
                free(temp);
                free(conn->data);
                free(conn->remaining);
                close(conn->sock);
                exit(EXIT_FAILURE);
            }
            conn->data = new_buf;
        }

        int i;
        for (i = 0; i < bytes_read; i++) {
            // copy temp to buf one byte at a time
            conn->data[data_size++] = temp[i];

            // check exit conditions
            if (num_bytes != 0){
                if (data_size == num_bytes){
                    conn->remaining_size = bytes_read-i-1;
                    memcpy(conn->remaining, temp+i+1, conn->remaining_size);
                    free(temp);
                    ack(conn->sock);
                    return;
                }
            } else if (conn->data[data_size-1] == delim){
                conn->data[data_size-1] = '\0';
                conn->remaining_size = bytes_read-i-1;
                memcpy(conn->remaining, temp+i+1, conn->remaining_size);
                free(temp);
                ack(conn->sock);
                return;
            }
        }
    }
}
