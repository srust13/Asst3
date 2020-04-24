#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "commands.h"

int server_fd;
project_t *projects;
pthread_mutex_t p_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

void cleanup(){
    puts("Initiating cleanup on server termination...");

    // close sockets
    close(server_fd);
    while (projects){
        project_t *next = projects->next;
        if (projects->sock)
            close(projects->sock);
        free(projects->name);
        free(projects);
        projects = next;
    }

    // free data
    puts("Done, thanks for waiting!");
}

void sigint_handler(int s){
    exit(EXIT_SUCCESS);
}

project_t *get_proj_info(char *project){

    // handle first project
    if (!projects){
        projects = calloc(1, sizeof(project_t));
        projects->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
        projects->name = strdup(project);
        return projects;
    }

    // search through linked list for matching project
    project_t *match = NULL;
    project_t *cur = projects;
    while (cur->next){
        if (!strcmp(cur->name, project)){
            match = cur;
            break;
        }
        cur = cur->next;
    }

    if (!match && !strcmp(cur->name, project))
        match = cur;

    if (!match){
        cur->next = calloc(1, sizeof(project_t));
        match = cur->next;
        match->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
        match->name = strdup(project);
    }
    return match;
}

void perform_cmd(int sock, char *cmd, char *proj){
    if (!strcmp(cmd, "checkout")){
        checkout(sock, proj);
    } else if (!strcmp(cmd, "update")){
        update(sock, proj);
    } else if (!strcmp(cmd, "upgrade")){
        upgrade(sock, proj);
    } else if (!strcmp(cmd, "commit")){
        commit(sock, proj);
    } else if (!strcmp(cmd, "push")){
        push(sock, proj);
    } else if (!strcmp(cmd, "create")){
        create(sock, proj);
    } else if (!strcmp(cmd, "destroy")){
        destroy(sock, proj);
    } else if (!strcmp(cmd, "currentversion")){
        currentversion(sock, proj);
    } else if (!strcmp(cmd, "history")){
        history(sock, proj);
    } else if (!strcmp(cmd, "rollback")){
        rollback(sock, proj);
    } else {
        printf("Invalid command: %s\n", cmd);
    }
}

void *handle_connection(void *sock_ptr){

    // initialize socket and per-thread random seed
    int sock = *((int *) sock_ptr);
    seed_rand();

    // read command
    char *command = recv_line(sock);

    // read client project. create if "create" command.
    char *project = set_create_project(sock, !strcmp(command, "create"));

    // perform project locking and then run the command
    if (project){
        pthread_mutex_lock(&p_lock);
        project_t *proj = get_proj_info(project);
        pthread_mutex_unlock(&p_lock);

        pthread_mutex_lock(&(proj->lock));
        perform_cmd(sock, command, project);
        pthread_mutex_unlock(&(proj->lock));
    }

    // cleanup
    free(command);
    free(project);
    close(sock);
    puts("Client disconnected");
    return 0;
}

int main(int argc, char *argv[]){

    // verify port arg
    if (argc < 2){
        puts("Missing port argument");
        exit(EXIT_FAILURE);
    }

    // register exit
    if (atexit(cleanup) != 0) {
        puts("atexit() registration failed");
        exit(EXIT_FAILURE);
    }

    // register sigint handler
    signal(SIGINT, sigint_handler);

    // initialize socket
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
    if (listen(server_fd, 100) < 0){
        puts("Couldn't listen on socket");
        exit(EXIT_FAILURE);
    }

    puts("Server started! Waiting for connections...");
    while(1){
        int client_fd = accept(server_fd, (struct sockaddr *) &client, &c);
        pthread_t thread_id;
        puts("Client connected");

        // if you want to debug a single threaded app,
        // just un-comment the below line
        // and comment out the if-statement and its contents entirely

        // handle_connection((void *) &client_fd);

        if(pthread_create(&thread_id, NULL,
                          handle_connection, (void *) &client_fd) < 0){
            puts("Couldn't create thread");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
