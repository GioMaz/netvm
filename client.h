#ifndef CLIENT_H
#define CLIENT_H

#include "program.h"

bool client_merge_all(int fd, Program *program);
bool client_insert(int fd, Program *program, uint32_t start);
bool client_exec(int fd);
void client_get_all(int fd, Program *program);
bool client_delete(int fd, uint32_t start, uint32_t size);
bool client_dump(int fd, int32_t *memory, uint32_t size);

#endif
