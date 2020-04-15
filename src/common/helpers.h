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

void sendline_ack(int sock, char *msg);

void send_file(char *filename, int sock);
void recv_file(int sock, char *fname);

int server_project_exists(int sock, char *project);
int recv_and_verify_project(buf_socket_t *conn);

void set_socket(int *sock);
void read_app_data_from_socket(buf_socket_t *conn, char delim, int num_bytes);
