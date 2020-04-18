#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "commands.h"

#define MAX_CONNS 10

int server_fd;
file_buf_t conn_infos[MAX_CONNS];
size_t free_threads[MAX_CONNS];
size_t free_threads_idx;
size_t num_free_threads;
pthread_mutex_t free_threads_lock;
pthread_cond_t free_threads_cond;

void cleanup(){
    puts("Initiating cleanup on server termination...");

    // wait for all threads to terminate
    pthread_mutex_lock(&free_threads_lock);
    while (num_free_threads < MAX_CONNS){
        pthread_cond_wait(&free_threads_cond, &free_threads_lock);
    }
    pthread_mutex_unlock(&free_threads_lock);

    // close socket
    close(server_fd);

    /*
        x shut down all threads
        - close all sockets
        - close all file descriptors
        - free all memory
    */
    puts("Done, thanks for waiting!");
}

void sigint_handler(int s){
    exit(EXIT_SUCCESS);
}

void perform_cmd(int sock){
    seed_rand();
    // read command
    char *command = recv_line(sock);

    // perform command
    if (!strcmp(command, "checkout")){
        checkout(sock);
    } else if (!strcmp(command, "update")){
        update(sock);
    } else if (!strcmp(command, "upgrade")){
        upgrade(sock);
    } else if (!strcmp(command, "commit")){
        commit(sock);
    } else if (!strcmp(command, "push")){
        push(sock);
    } else if (!strcmp(command, "create")){
        create(sock);
    } else if (!strcmp(command, "destroy")){
        destroy(sock);
    } else if (!strcmp(command, "currentversion")){
        currentversion(sock);
    } else if (!strcmp(command, "history")){
        history(sock);
    } else if (!strcmp(command, "rollback")){
        rollback(sock);
    } else {
        printf("Invalid command: %s\n", command);
        free(command);
        close(sock);
        exit(EXIT_FAILURE);
    }
    free(command);
}

void *handle_connection(void *conninfo_idx){

    // get socket fd
    int conn_idx = *((int *) conninfo_idx);
    file_buf_t *conn = &(conn_infos[conn_idx]);

    // handle request
    perform_cmd(conn->sock);

    // cleanup
    close(conn->sock);

    puts("Client disconnected");
    pthread_mutex_lock(&free_threads_lock);
    size_t insert_idx = (conn_idx + num_free_threads++) % MAX_CONNS;
    free_threads[insert_idx] = conn_idx;
    pthread_mutex_unlock(&free_threads_lock);
    pthread_cond_signal(&free_threads_cond);
    return 0;
}

void init(){
    // initialize locks and condition vars
    free_threads_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    free_threads_cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

    // add all indexes to free threads buffer
    pthread_mutex_lock(&free_threads_lock);
    int i;
    for (i = 0; i < MAX_CONNS; i++){
        free_threads[i] = i;
    }
    num_free_threads = MAX_CONNS;
    pthread_mutex_unlock(&free_threads_lock);

    // register exit
    if (atexit(cleanup) != 0) {
        puts("atexit() registration failed");
        exit(EXIT_FAILURE);
    }

    // register sigint handler
    signal(SIGINT, sigint_handler);
}

int main(int argc, char *argv[]){
    // initialize pthreads and exit handler
    init();

    // verify port arg
    if (argc < 2){
        puts("Missing port argument");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client, server;
    socklen_t c = sizeof(struct sockaddr_in);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(argv[1]));

    if (server.sin_port == 0 || server.sin_port > 65535) {
        puts("Invalid port argument");
        exit(EXIT_FAILURE);
    }

    // create, bind, and listen on socket
    int enable = 1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        puts("Couldn't create socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        puts("setsockopt(SO_REUSEADDR) failed");
    }
    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0){
        puts("Couldn't bind to port");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAX_CONNS) < 0){
        puts("Couldn't listen on socket");
        exit(EXIT_FAILURE);
    }

    puts("Server started! Waiting for connections...");
    while(1){
        int client_fd = accept(server_fd, (struct sockaddr *) &client, &c);

        // get new free thread spot
        pthread_mutex_lock(&free_threads_lock);
        while (num_free_threads <= 0){
            pthread_cond_wait(&free_threads_cond, &free_threads_lock);
        }

        int conninfo_idx = free_threads[free_threads_idx];
        free_threads_idx = (free_threads_idx + 1) % MAX_CONNS;
        num_free_threads--;
        puts("Client connected");
        pthread_mutex_unlock(&free_threads_lock);

        // kick off pthread to handle client
        conn_infos[conninfo_idx].sock = client_fd;

        // if you want to debug a single threaded app,
        // just un-comment the below line
        // and comment out the if-statement and its contents entirely

        handle_connection((void *) &conninfo_idx);

        // if(pthread_create(&(conn_infos[conninfo_idx].thread_id), NULL,
        //                   handle_connection, (void *) &conninfo_idx) < 0){
        //     puts("Couldn't create thread");
        //     exit(EXIT_FAILURE);
        // }
    }
    return 0;
}
