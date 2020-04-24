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
    char code;
    int  version;
    char hexdigest[32+1];
    char *fname;

    struct manifest_line_t *next;
} manifest_line_t;

int file_exists_local(char *project, char *fname);
void mkpath(char* file_path);
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
void md5sum(char *filename, char *hexstring);
void assert_project_exists_local(char *project);
void init_socket_server(int *sock, char *command);
int server_project_exists(int sock, char *project);
char *set_create_project(int sock, int should_create);
void gen_temp_filename(char *tempfile);

void add_to_manifest(char *project, char *filenames);
void remove_from_manifest(char *project, char *filenames);
char *search_file_in_manifest(char *manifest, char *search);
char *generate_manifest_line(char code, char *hexdigest, int version, char *fname);
manifest_line_t *parse_manifest_line(char *line);
void clean_manifest_line(manifest_line_t *ml);
int generate_commit_file(char *commit, char *client_manifest, char *server_manifest);
char* generate_am_tar(char *commitPath);
void regenerate_manifest(char *client_manifest, char *commit);
int get_manifest_version(char *manifest);

char* gen_commit_filename(char *project);
void remove_all_commits(char *project);
void update_repo_from_commit(char *commit);
char *commit_exists(char *project, char *client_hex);

void generate_update_conflict_files(char *project, char *client_manifest, char *server_manifest);

void rollback_every_file(char *project, char *version);
