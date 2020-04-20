#pragma once

#include "../common/helpers.h"

void checkout(int sock, char *project);
void update(int sock, char *project);
void upgrade(int sock, char *project);
void commit(int sock, char *project);
void push(int sock, char *project);
void create(int sock, char *project);
void destroy(int sock, char *project);
void currentversion(int sock, char *project);
void history(int sock, char *project);
void rollback(int sock, char *project);
