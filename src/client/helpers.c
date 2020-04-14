#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "helpers.h"

void assert_malloc_success(void *buf, int count, ...) {
    if (buf == NULL){

        // free all buffers passed into function
        va_list ap;
        va_start(ap, count);
        int i;
        for (i = 0; i < count; i++){
            free(va_arg(ap, void *));
        }
        va_end(ap);

        puts("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * Reads a chunk of data from a file.
 * The returned pointer should be freed after it has been used.
 */
char *read_chunk(int fd, int *bytes_read, int *eof){

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
        char *temp = read_chunk(fd, &bytes_read, &eof);
        int i;
        for (i = 0; i < bytes_read; i++){
            // double buffer size if full
            if (buf_idx == buf_len){
                buf_len *= 2;
                char *new_buf = realloc(buf, buf_len);
                assert_malloc_success(new_buf, 2, buf, temp);
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

    puts("Successfully connected to server");
}

/**
 * Send newline-delimited message to server.
 */
void send_msg(int sock, char *msg){
    write(sock, msg, strlen(msg));
    write(sock, "\n", 1);
}
