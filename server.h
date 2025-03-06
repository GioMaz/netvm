#ifndef SERVER_H
#define SERVER_H

#include "el.h"

#define PAYLOAD_SIZE (2 * sizeof(Instruction))

_Static_assert(
    PAYLOAD_SIZE > sizeof(Instruction),
    "PAYLOAD_SIZE should be greater than sizeof(Instruction)"
);

typedef enum {
    MERGE,
    INSERT,
    EXEC,
    RESET,
    GET,
    DELETE,
    DUMP,
} Method;

typedef struct {
    int32_t type; // enum Method
    uint32_t size;
} RequestHeader;

_Static_assert(
    sizeof(((RequestHeader) {}).type) == sizeof(Method),
    "RequestHeader.type sould have the same size as Method"
);

typedef struct {
    RequestHeader header;
    uint8_t payload[PAYLOAD_SIZE];
} Request;

typedef enum {
    SUCCESS,
    FAILURE,
    UNKNOWN_METHOD,
} Status;

typedef struct {
    int32_t status; // enum Status
    uint32_t size;
} ResponseHeader;

typedef struct {
    ResponseHeader header;
    uint8_t payload[PAYLOAD_SIZE];
} Response;

bool handle_connection(Conn *conn);
bool handle_request(Conn *conn);
ConnState handle_merge(Conn *conn, Request *req, Response *res);
ConnState handle_insert(Conn *conn, Request *req, Response *res);
ConnState handle_exec(Conn *conn, Response *res);
ConnState handle_reset(Conn *conn, Response *res);
ConnState handle_get(Conn *conn, Request *req, Response *res);
ConnState handle_delete(Conn *conn, Request *req, Response *res);
ConnState handle_dump(Conn *conn, Request *req, Response *res);
bool handle_response(Conn *conn);
void handle_loop(Conn *conn);
void start_server(uint16_t port);

#endif
