#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

void die(char *s)
{
    fprintf(stderr, "%s", s);
    exit(1);
}

bool read_all(int fd, void *buf, size_t n)
{
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return false;
        }
        n -= rv;
        buf += rv;
    }
    return true;
}

bool write_all(int fd, void *buf, size_t n)
{
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return false;
        }
        n -= rv;
        buf += rv;
    }
    return true;
}

void set_nonblocking(int fd)
{
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        fprintf(stderr, "Failed to get fd flags\n");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        fprintf(stderr, "Failed to set fd flags\n");
    }
}
