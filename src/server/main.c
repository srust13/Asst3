#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CONNS 10

typedef struct connection_info {
    pthread_t thread_id;
    int client_fd;
} connection_info;

int server_fd;
connection_info conn_infos[MAX_CONNS];
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

void *client_handler(void *conninfo_idx){

    int conn_idx = *((size_t *)conninfo_idx);
    connection_info conn = conn_infos[conn_idx];

    //Get the socket descriptor
    int sock = conn.client_fd;
    int read_size;
    char client_message[2000];

    //Send some messages to the client
    char *message = "Greetings! I am your connection handler\n";
    write(sock, message, strlen(message));

    message = "Now type something and i shall repeat what you type \n";
    write(sock, message, strlen(message));

    //Receive a message from client
    while( (read_size = recv(sock, client_message, 2000, 0)) > 0 ){
        //end of string marker
		client_message[read_size] = '\0';

		//Send the message back to client
        write(sock, client_message, strlen(client_message));

		//clear the message buffer
		memset(client_message, 0, 2000);
    }

    if(read_size == 0){
        puts("Client disconnected");
        fflush(stdout);
    } else if(read_size == -1){
        puts("recv failed");
    }

    // cleanup
    close(sock);

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
    for (size_t i = 0; i < MAX_CONNS; i++){
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
        conn_infos[conninfo_idx].client_fd = client_fd;
        if(pthread_create(&(conn_infos[conninfo_idx].thread_id), NULL,
                          client_handler, (void *) &conninfo_idx) < 0){
            puts("Couldn't create thread");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
