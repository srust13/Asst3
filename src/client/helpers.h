#pragma once

#define CHUNK_SIZE 1024

char *read_chunk(int fd, int *bytes_read, int *eof);
void set_socket(int *sock);
void send_msg(int sock, char *msg);
