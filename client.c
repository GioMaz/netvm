#include <stdio.h>
#include <string.h>

#include "program.h"
#include "server.h"
#include "utils.h"

bool client_merge_all(int fd, Program *program)
{
    Request req;
    Response res;

    size_t count = 0;

    while (program_size(program)) {
        size_t n = MIN(
            program_size(program),
            PAYLOAD_SIZE/sizeof(Instruction)
        ); // MIN(9, 16);
        size_t size = n * sizeof(Instruction);

        program_split(program, (Instruction *)req.payload, n);
        req.header = (RequestHeader) {
            .type = MERGE,
            .size = size
        };
        size_t req_size = sizeof(req.header) + req.header.size;
        write_all(fd, &req, req_size);
        count++;
    }

    for (size_t i = 0; i < count; i++) {
        read_all(fd, &res.header, sizeof(res.header));
        if (res.header.status == FAILURE)
            return false;
        read_all(fd, res.payload, res.header.size);
    }

    return true;
}

bool client_insert(int fd, Program *program, uint32_t start)
{
    Request req;
    Response res;

    size_t count = 0;

    while (program_size(program)) {
        size_t n = MIN(
            program_size(program),
            ((PAYLOAD_SIZE - 2 * sizeof(uint64_t))/sizeof(Instruction))
        );
        size_t bytes = n * sizeof(Instruction);

        ((uint64_t *)req.payload)[0] = (uint64_t)start;
        ((uint64_t *)req.payload)[1] = (uint64_t)n;
        program_split(program, &((Instruction *)req.payload)[1], n);
        req.header = (RequestHeader) {
            .type = INSERT,
            .size = 2 * sizeof(uint64_t) + bytes,
        };
        size_t req_size = sizeof(req.header) + req.header.size;
        write_all(fd, &req, req_size);
        start += n;
        count++;
    }

    for (size_t i = 0; i < count; i++) {
        read_all(fd, &res.header, sizeof(res.header));
        if (res.header.status == FAILURE)
            return false;
        read_all(fd, res.payload, res.header.size);
    }

    return true;
}

void client_get_all(int fd, Program *program)
{
    uint32_t offset = 0;
    const uint32_t chunk = PAYLOAD_SIZE / sizeof(Instruction);

    Request req;
    Response res;

    do {
        req.header = (RequestHeader) {
            .type = GET,
            .size = 2 * sizeof(uint32_t),
        };
        ((uint32_t *)req.payload)[0] = offset;
        ((uint32_t *)req.payload)[1] = chunk;
        write_all(fd, &req, sizeof(req.header) + req.header.size);

        read_all(fd, &res, sizeof(res.header));
        read_all(fd, res.payload, res.header.size);
        program_merge(program, (Instruction *)res.payload,
                res.header.size / sizeof(Instruction));

        offset += chunk;
    } while (res.header.status == SUCCESS);
}

bool client_exec(int fd)
{
    Request req;
    req.header = (RequestHeader) {
        .type = EXEC,
        .size = 0,
    };
    write_all(fd, &req, sizeof(req.header));

    Response res;
    read_all(fd, &res, sizeof(res.header));
    if (res.header.status == FAILURE)
        return false;
    read_all(fd, res.payload, res.header.size);
    return true;
}

bool client_delete(int fd, uint32_t start, uint32_t size)
{
    Request req;
    req.header = (RequestHeader) {
        .type = DELETE,
        .size = 2 * sizeof(uint32_t),
    };
    ((uint32_t *)req.payload)[0] = start;
    ((uint32_t *)req.payload)[1] = size;
    write_all(fd, &req, sizeof(req.header) + req.header.size);

    Response res;
    read_all(fd, &res, sizeof(res.header));
    if (res.header.status == FAILURE)
        return false;
    read_all(fd, res.payload, res.header.size);
    return true;
}

bool client_dump(int fd, int32_t *memory, uint32_t size)
{
    Response res;
    Request req;

    size_t offset = 0;

    while (size > 0) {
        req.header = (RequestHeader) {
            .type = DUMP,
            .size = 2 * sizeof(uint32_t),
        };
        ((uint32_t *)req.payload)[0] = offset;
        ((uint32_t *)req.payload)[1] = size;
        write_all(fd, &req, sizeof(req.header) + req.header.size);

        read_all(fd, &res, sizeof(res.header));
        if (res.header.status == FAILURE)
            return false;
        read_all(fd, res.payload, res.header.size);
        size_t n = res.header.size / sizeof(int);

        memcpy(&memory[offset], &res.payload, res.header.size);

        offset += n;
        size -= n;
    }

    return true;
}
