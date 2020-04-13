#pragma once

void configure(char *hostname, char *port);
void checkout(char *project);
void update(char *project);
void upgrade(char *project);
void commit(char *project);
void push(char *project);
void create(char *project);
void destroy(char *project);
void add(char *project, char *filename);
void remove_cmd(char *project, char *filename);
void currentversion(char *project);
void history(char *project);
void rollback(char *project, char *version);
