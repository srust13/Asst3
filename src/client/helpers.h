#pragma once

char *read_chunk(int fd, int *bytes_read, int *eof);
void read_and_test_config(int *sock);
