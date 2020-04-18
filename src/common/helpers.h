#pragma once
#include <pthread.h>

#define CHUNK_SIZE 1024

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


void sendline(int sock, char *msg);
char *recvline(int sock);
void read_line(file_buf_t *info);
void send_file(char *filename, int sock);
void recv_file(int sock, char *dest);
void add_files_to_manifest(char **filenames, int num_files);
void remove_files_from_manifest(char **filenames, int num_files);
void assert_project_exists_local(char *project);
void init_socket_server(int *sock, char *command);
int server_project_exists(int sock, char *project);
int set_create_project(int sock, int should_create);
void gen_temp_filename(char *tempfile);
