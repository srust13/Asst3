#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#include "helpers.h"

/**
 * Reads chunks from socket until num_bytes has been read, or if
 * num_bytes is 0, then until delimiter is read. Any bytes left
 * in the chunk after the exit condition has been reached are
 * placed in conn->remaining. Delimiters are replaced with a
 * null byte.
 *
 * Note:
 * It is possible that this function doesn't read from
 * the socket at all. This occurs if there is existing data in
 * the "remaining" already satisfies the exit condition.
 */
void read_chunks(buf_socket_t *conn, char delim, int num_bytes){

    // copy leftover data from "remaining" into "data"
    memcpy(conn->data, conn->remaining, conn->remaining_size);
    int data_size = conn->remaining_size;
    conn->remaining_size = 0;

    // check if exit condition reached already
    if (num_bytes != 0){
        if (data_size >= num_bytes){
            conn->remaining_size = data_size - num_bytes;
            memcpy(conn->remaining, conn->data + num_bytes, conn->remaining_size);
            return;
        }
    } else {
        int i;
        for (i = 0; i < data_size; i++){
            if (conn->data[i] == delim){
                conn->data[i] = '\0';
                conn->remaining_size = data_size - i - 1;
                memcpy(conn->remaining, conn->data + i + 1, conn->remaining_size);
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
                    return;
                }
            } else if (conn->data[data_size-1] == delim){
                conn->data[data_size-1] = '\0';
                conn->remaining_size = bytes_read - i;
                memcpy(conn->remaining, temp+i+1, conn->remaining_size);
                free(temp);
                return;
            }
        }
    }
}
