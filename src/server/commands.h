#pragma once

#include "../common/helpers.h"

void checkout(int sock);
void update(int sock);
void upgrade(int sock);
void commit(int sock);
void push(int sock);
void create(int sock);
void destroy(int sock);
void currentversion(int sock);
void history(int sock);
void rollback(int sock);
