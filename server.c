#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "el.h"
#include "utils.h"
#include "server.h"

bool handle_connection(Conn *conn)
{
    switch (conn->state) {
        case CONN_REQ:
            handle_request(conn);
            break;
        case CONN_RES:
            handle_response(conn);
            break;
        case CONN_LOOP:
            handle_loop(conn);
            break;
        case CONN_END:
            return false;
    }

    return true;
}

bool handle_request(Conn *conn)
{
    while (conn->state == CONN_REQ) {
        ssize_t bytes = 0;
        do {
            size_t n = sizeof(conn->rbuf) - conn->rbuf_size;
            bytes = read(conn->fd, conn->rbuf, n);
        } while (bytes < 0 && errno == EINTR);

        // Check if the file is ready for polling
        if (bytes < 0) {
            if (errno != EAGAIN) {
                printf("Failed to read from request buffer\n");
                conn->state = CONN_END;
                return false;
            }
            break;
        }

        // Check if the request has finished
        if (bytes == 0) {
            if (conn->rbuf_size > 0) {
                fprintf(stderr, "Unexpected EOF\n");
                exit(1);
                return false;
            } else {
                printf("EOF\n");
            }
            conn->state = CONN_END;
            break;
        }

        conn->rbuf_size += (size_t)bytes;

        // Use this for pipelining, allow for multiple
        // requests to be stored inside rbuf simultaneously
        while (conn->rbuf_size) {
            // Check if the header is ready to be read
            if (conn->rbuf_size < sizeof(RequestHeader)) {
                break;
            }

            // Read header
            RequestHeader header;
            memcpy(&header, conn->rbuf, sizeof(header));

            // Check if the payload is ready to be read
            if (conn->rbuf_size - sizeof(header) < header.size) {
                break;
            }

            // Read payload
            Request req = { header };
            memcpy(req.payload, conn->rbuf + sizeof(header), header.size);

            Response res = {0};
            switch (req.header.type) {
                case MERGE:
                    conn->state = handle_merge(conn, &req, &res);
                    break;
                case INSERT:
                    conn->state = handle_insert(conn, &req, &res);
                    break;
                case EXEC:
                    conn->state = handle_exec(conn, &res);
                    break;
                case RESET:
                    conn->state = handle_reset(conn, &res);
                    break;
                case GET:
                    conn->state = handle_get(conn, &req, &res);
                    break;
                case DELETE:
                    conn->state = handle_delete(conn, &req, &res);
                    break;
                case DUMP:
                    conn->state = handle_dump(conn, &req, &res);
                    break;
                default:
                    res.header.status = UNKNOWN_METHOD;
                    res.header.size = 0;
                    break;
            }

            // Copy response to wbuf
            size_t res_size = sizeof(res.header) + res.header.size;
            memcpy(conn->wbuf, &res, res_size);
            conn->wbuf_size = res_size;

            // Clear rbuf
            size_t req_size = sizeof(req.header) + req.header.size;
            size_t remain = conn->rbuf_size - req_size;
            if (remain) {
                memmove(conn->rbuf, conn->rbuf + req_size, remain);
            }
            conn->rbuf_size = remain;

            if (conn->state == CONN_RES || conn->state == CONN_LOOP) {
                handle_response(conn);
            }
        }
    }

    return true;
}

ConnState handle_merge(Conn *conn, Request *req, Response *res)
{
    printf("MERGE...\n");
    size_t bytes = req->header.size;
    size_t n = bytes / sizeof(Instruction);

    Program *program = conn->vm->program;
    bool rv = program_merge(program, (Instruction *)req->payload, n);
    if (!rv) {
        printf("Failed to merge program\n");
        res->header.status = FAILURE;
        res->header.size = 0;
    } else {
        res->header.status = SUCCESS;
        res->header.size = 0;
        conn->state = CONN_RES;
    }

    return CONN_RES;
}

ConnState handle_insert(Conn *conn, Request *req, Response *res)
{
    printf("INSERT...\n");
    Instruction *insts = (Instruction *)req->payload;
    uint64_t start = ((uint64_t *)insts)[0];
    uint64_t size = ((uint64_t *)insts)[1];

    Instruction *src = &insts[1];
    Program *program = conn->vm->program;
    bool rv = program_insert(program, src, start, size);
    if (!rv) {
        printf("Failed to insert to program\n");
        res->header.status = FAILURE;
        res->header.size = 0;
    } else {
        res->header.status = SUCCESS;
        res->header.size = 0;
    }

    return CONN_RES;
}

ConnState handle_exec(Conn *conn, Response *res)
{
    printf("EXEC...\n");
    vm_setreg(conn->vm);
    res->header.status = SUCCESS;
    res->header.size = 0;
    return CONN_LOOP;
}

ConnState handle_reset(Conn *conn, Response *res)
{
    printf("RESET...\n");
    bool rv = program_clear(conn->vm->program);
    if (!rv) {
        printf("Failed to reset program\n");
        res->header.status = FAILURE;
        res->header.size = 0;
    } else {
        vm_setreg(conn->vm);
        res->header.status = SUCCESS;
        res->header.size = 0;
    }

    return CONN_RES;
}

ConnState handle_get(Conn *conn, Request *req, Response *res)
{
    printf("GET...\n");
    size_t start = ((uint32_t *)req->payload)[0];
    size_t size = ((uint32_t *)req->payload)[1];
    size = MIN(size, PAYLOAD_SIZE / sizeof(Instruction));

    Program *program = conn->vm->program;
    size_t n = program_get(program, (Instruction *)res->payload, start, size);
    if (n) {
        res->header.status = SUCCESS;
        res->header.size = n * sizeof(Instruction);
    } else {
        res->header.status = FAILURE;
        res->header.size = 0;
    }

    return CONN_RES;
}

ConnState handle_delete(Conn *conn, Request *req, Response *res)
{
    printf("DELETE...\n");
    size_t start = ((uint32_t *)req->payload)[0];
    size_t size = ((uint32_t *)req->payload)[1];
    Program *program = conn->vm->program;
    uint32_t n = program_delete(program, start, size);

    if (n) {
        res->header.status = SUCCESS;
        res->header.size = sizeof(n);
        ((uint32_t *)res->payload)[0] = n;
    } else {
        printf("Failed to delete program\n");
        res->header.status = FAILURE;
        res->header.size = 0;
    }

    return CONN_RES;
}

ConnState handle_dump(Conn *conn, Request *req, Response *res)
{
    printf("DUMP...\n");
    size_t start = ((uint32_t *)req->payload)[0];
    size_t size = ((uint32_t *)req->payload)[1];
    int *memory = conn->vm->memory;
    size = MIN(size, PAYLOAD_SIZE / sizeof(memory[0]));

    if (start + size < MEMORY_SIZE) {
        res->header.status = SUCCESS;
        res->header.size = size * sizeof(memory[0]);
        memcpy(res->payload, &memory[start], res->header.size);
    } else {
        printf("Failed to get memory dump\n");
        res->header.status = FAILURE;
        res->header.size = 0;
    }

    return CONN_RES;
}

bool handle_response(Conn *conn)
{
    while (conn->wbuf_sent < conn->wbuf_size) {
        size_t bytes = 0;
        do {
            size_t n = conn->wbuf_size - conn->wbuf_sent;
            bytes = write(conn->fd, conn->wbuf + conn->wbuf_sent, n);
        } while (bytes < 0 && errno == EINTR);

        if (bytes < 0) {
            if (errno != EAGAIN) {
                printf("Failed to write to response buffer\n");
                conn->state = CONN_END;
                return false;
            }
            break;
        }

        conn->wbuf_sent += (size_t)bytes;

        assert(conn->wbuf_sent <= conn->wbuf_size);
    }

    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;

    if (conn->state == CONN_RES) {
        conn->state = CONN_REQ;
    }

    return true;
}

bool handle_loop(Conn *conn)
{
    if (loopn(conn->vm)) {
        conn->state = CONN_REQ;
    }
    return true;
}

void start_server(uint16_t port)
{
    // 1) socket()
    int welcfd = socket(AF_INET, SOCK_STREAM, 0);
    if (welcfd < 0)
        die("Failed to create welcome socket\n");

    int val = 1;
    setsockopt(welcfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // 2) bind()
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(welcfd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0)
        die("Failed to bind address to socket\n");

    // 3) listen()
    listen(welcfd, SOMAXCONN);
    if (rv < 0)
        die("Failed to listen from welcome socket\n");
    else
        printf("Listening on port %d...\n", port);

    set_nonblocking(welcfd);

    EventLoop el;
    el_init(&el);

    // Pollfd Array
    struct pollfd pa[MAX_CONN];
    struct pollfd pfd = {welcfd, POLLIN, 0};
    pa[0] = pfd;

    while (1) {
        size_t pa_size;
        if (!el_get_pa(&el, pa, &pa_size)) {
            printf("Failed to get poll array from event loop\n");
        }

        if (poll(pa, (nfds_t)pa_size, 1000) < 0) {
            printf("Failed to poll fds\n");
        }

        for (size_t i = 1; i < pa_size; i++) {
            if (pa[i].revents) {
                int fd = pa[i].fd;
                Conn *conn = el_get(&el, fd);
                if (!handle_connection(conn)) {
                    el_remove(&el, fd);
                }
            }
        }

        if (pa[0].revents) {
            struct sockaddr_in client_addr = {0};
            socklen_t socklen = sizeof(client_addr);
            int connfd = accept(welcfd, (struct sockaddr *)&client_addr, &socklen);
            if (connfd < 0) {
                fprintf(stderr, "Failed to accept new connection\n");
                exit(1);
            }

            set_nonblocking(connfd);

            el_add(&el, connfd);
        }
    }

    close(welcfd);
}
