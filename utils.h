#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdbool.h>

#define MIN(a, b) a < b ? a : b

void die(char *s);
bool read_all(int fd, void *buf, size_t n);
bool write_all(int fd, void *buf, size_t n);
void set_nonblocking(int fd);

#endif
