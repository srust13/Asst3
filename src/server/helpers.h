#pragma once
#include <pthread.h>

#define CHUNK_SIZE 1024

typedef struct buf_socket_t {
    char *data;
    char *remaining;

    int data_buf_size;
    int remaining_size;

    int sock;
    pthread_t thread_id;
} buf_socket_t;

void read_chunks(buf_socket_t *conn, char delim, int num_bytes);
