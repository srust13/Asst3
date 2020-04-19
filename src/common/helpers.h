#pragma once
#include <pthread.h>

#define CHUNK_SIZE 1024

void seed_rand();

typedef struct file_buf_t {
    char *data;
    char *remaining;

    int data_buf_size;
    int remaining_size;

    int sock;
    int fd;
    int file_eof;
    pthread_t thread_id;
} file_buf_t;

void init_file_buf(file_buf_t *info, char *filename);
void clean_file_buf(file_buf_t *info);

void send_line(int sock, char *msg);
char *recv_line(int sock);
void read_file_until(file_buf_t *info, char delim);
void send_file(char *filename, int sock, int send_filename);
void recv_file(int sock, char *dest);
void send_directory(char *dirname, int sock);
void recv_directory(int sock, char *dirname);
void add_to_manifest(char *project, char *filenames);
void remove_from_manifest(char *project, char *filenames);
void assert_project_exists_local(char *project);
void init_socket_server(int *sock, char *command);
int server_project_exists(int sock, char *project);
char *set_create_project(int sock, int should_create);
void gen_temp_filename(char *tempfile);
