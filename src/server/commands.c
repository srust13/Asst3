#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/helpers.h"

void checkout(int sock){
    puts("Checkout");
}

void update(int sock){
    puts("Update");
}

void upgrade(int sock){
    puts("Upgrade");
}

void commit(int sock){
    puts("Commit");
}

void push(int sock){
    puts("Push");
}

void create(int sock){
    // if project doesn't exist, create it
    if (set_create_project(sock, 1))
        return;

    // send requested .Manifest to client
    send_file(".Manifest", sock);
}

void destroy(int sock){
    puts("Destroy");
}

void currentversion(int sock){
    if (!set_create_project(sock, 0)){
        puts("Project does not exist on server.");
        return;
    }

    // send requested .Manifest to client
    send_file(".Manifest", sock);
}

void history(int sock){
    puts("History");
}

void rollback(int sock){
    puts("Rollback");
}
