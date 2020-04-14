#pragma once

#include "helpers.h"

void checkout(buf_socket_t *conn);
void update(buf_socket_t *conn);
void upgrade(buf_socket_t *conn);
void commit(buf_socket_t *conn);
void push(buf_socket_t *conn);
void create(buf_socket_t *conn);
void destroy(buf_socket_t *conn);
void add(buf_socket_t *conn);
void remove_cmd(buf_socket_t *conn);
void currentversion(buf_socket_t *conn);
void history(buf_socket_t *conn);
void rollback(buf_socket_t *conn);
