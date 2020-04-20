#pragma once
#include <pthread.h>

#define CHUNK_SIZE 1024

void seed_rand();

typedef struct project_t {
    pthread_mutex_t lock;
    int sock;
    char *name;
    struct project_t *next;
} project_t;

typedef struct file_buf_t {
    char *data;
    char *remaining;

    int data_buf_size;
    int remaining_size;

    int fd;
    int file_eof;
} file_buf_t;

typedef struct manifest_line_t {
    char *code;
    char *version;
    char *hexdigest;
    char *fname;
} manifest_line_t;

int file_exists_local(char *project, char *fname);
void init_file_buf(file_buf_t *info, char *filename);
void clean_file_buf(file_buf_t *info);

void send_int(int sock, int num);
int recv_int(int sock);
void send_line(int sock, char *msg);
char *recv_line(int sock);
void read_file_until(file_buf_t *info, char delim);
void send_file(char *filename, int sock, int send_filename);
void recv_file(int sock, char *dest);
void send_directory(int sock, char *dirname);
void recv_directory(int sock, char *dirname);
void assert_project_exists_local(char *project);
void init_socket_server(int *sock, char *command);
int server_project_exists(int sock, char *project);
char *set_create_project(int sock, int should_create);
void gen_temp_filename(char *tempfile);

void add_to_manifest(char *project, char *filenames);
void remove_from_manifest(char *project, char *filenames);
char *search_file_in_manifest(char *manifest, char *search);
char *generate_manifest_line(char *code, char *hexdigest, char *version, char *fname);
void parse_manifest_line(manifest_line_t *ml, char *line);
void clean_manifest_line(manifest_line_t *ml);
int generate_commit_file(char *commit, char *client_manifest, char *server_manifest);
char* generate_am_tar(char *commitPath);
void regenerate_manifest(char *client_manifest);
