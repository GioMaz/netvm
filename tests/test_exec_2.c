#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "../program.h"
#include "../server.h"
#include "../client.h"
#include "tests.h"

void test_exec_2()
{
    int pid = fork();
    if (pid) {
        usleep(500);
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(PORT);
        addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
        connect(fd, (struct sockaddr *)&addr, sizeof(addr));

        Program program_1;
        Program program_2;
        program_init(&program_1);
        program_init(&program_2);

        Instruction i0 = { MOVI,    R0, 5 };
        Instruction i1 = { MOV,     R1, R0 };
        Instruction i2 = { ADDI,    R1, R1, -1 };
        Instruction i3 = { BEQI,    6, R1, 1 };
        Instruction i4 = { MUL,     R0, R0, R1 };
        Instruction i5 = { B,       2 };
        Instruction i6 = { HALT };
        Instruction iX = { B,       0 };

        program_add(&program_1, i0);
        program_add(&program_1, iX);
        program_add(&program_1, iX);
        program_add(&program_1, i1);
        program_add(&program_1, i2);
        program_add(&program_1, i3);
        program_add(&program_1, i4);
        program_add(&program_1, i5);
        program_add(&program_1, i6);

        program_clone(&program_2, &program_1);

        client_merge_all(fd, &program_1);

        // Delete remotely
        client_delete(fd, 1, 2);

        // Delete locally
        program_delete(&program_2, 1, 2);

        client_get_all(fd, &program_1);

        char *error = NULL;
        for (size_t i = 0; i < program_2.size; i++) {
            Instruction *inst_1 = program_fetch(&program_1, i);
            Instruction *inst_2 = program_fetch(&program_2, i);
            if (inst_1->code != inst_2->code)
                error = "Received opcode does not match";
            else if (inst_1->dest != inst_2->dest)
                error = "Received opcode does not match";
            else if (inst_1->arg1 != inst_2->arg1)
                error = "Received arg1 does not match";
            else if (inst_1->arg2 != inst_2->arg2)
                error = "Received arg2 does not match";
        }

        client_exec(fd);

        kill(pid, SIGQUIT);

        if (error) {
            printf("%s\n", error);
            assert(false);
        }
    } else {
        /*freopen("/dev/null", "w", stdout);*/
        start_server(PORT);
    }
}

/*    // Create program (factorial of 5)*/
/*    Program program;*/
/*    program_init(&program);*/
/**/
/*    Instruction i1 = { MOVI,    0, 5 };*/
/*    Instruction i2 = { MOV,     1, 0 };*/
/*    Instruction i3 = { ADDI,    1, 1, -1};*/
/*    Instruction i4 = { BEQI,    6, 1, 1};*/
/*    Instruction i5 = { MUL,     0, 0, 1};*/
/*    Instruction i6 = { B,       2 };*/
/*    Instruction i7 = { HALT };*/
/*    Instruction iX = { B,    0 };*/
/**/
/*    program_add(&program, i1);*/
/*    program_add(&program, iX);*/
/*    program_add(&program, iX);*/
/*    program_add(&program, i2);*/
/*    program_add(&program, i3);*/
/*    program_add(&program, i4);*/
/*    program_add(&program, i5);*/
/*    program_add(&program, i6);*/
/*    program_add(&program, i7);*/
/**/
/*    Request req;*/
/*    Response res;*/
/**/
/*    // MERGE message*/
/*    size_t count = 0;*/
/**/
/*    while (program_size(&program)) {*/
/*        size_t n = MIN(program_size(&program),*/
/*                PAYLOAD_SIZE/sizeof(Instruction)); // MIN(9, 16);*/
/**/
/*        size_t size = n * sizeof(Instruction);*/
/**/
/*        program_split(&program, (Instruction *)req.payload, n);*/
/*        req.header = (RequestHeader) {*/
/*            .type = MERGE,*/
/*            .size = size*/
/*        };*/
/*        size_t req_size = sizeof(req.header) + req.header.size;*/
/*        write_all(fd, &req, req_size);*/
/*        count++;*/
/*    }*/
/**/
/*    program_deinit(&program);*/
/**/
/*    // Responses*/
/*    for (size_t i = 0; i < count; i++) {*/
/*        read_all(fd, &res.header, sizeof(res.header));*/
/*        read_all(fd, res.payload, res.header.size);*/
/*    }*/
/**/
/*    // DELETE message*/
/*    req.header = (RequestHeader) {*/
/*        .type = DELETE,*/
/*        .size = 2 * 4*/
/*    };*/
/*    ((uint32_t *)req.payload)[0] = 1;*/
/*    ((uint32_t *)req.payload)[1] = 2;*/
/*    write_all(fd, &req, sizeof(req.header) + req.header.size);*/
/**/
/*    // Response*/
/*    read_all(fd, &res.header, sizeof(res.header));*/
/*    read_all(fd, res.payload, res.header.size);*/
/**/
/*    // GET message*/
/*    req.header = (RequestHeader) {*/
/*        .type = GET,*/
/*        .size = 2 * 4*/
/*    };*/
/*    ((uint32_t *)req.payload)[0] = 0;*/
/*    ((uint32_t *)req.payload)[1] = 4;*/
/*    write_all(fd, &req, sizeof(req.header) + req.header.size);*/
/**/
/*    // Response*/
/*    Program tmp;*/
/*    program_init(&tmp);*/
/*    read_all(fd, &res, sizeof(res.header));*/
/*    read_all(fd, res.payload, res.header.size);*/
/*    program_merge(&tmp, (Instruction *)res.payload, res.header.size / sizeof(Instruction));*/
/*    program_deinit(&tmp);*/
/**/
/*    // EXEC message*/
/*    req.header = (RequestHeader) {*/
/*        .type = EXEC,*/
/*        .size = 0*/
/*    };*/
/*    write_all(fd, &req, sizeof(req.header));*/
/**/
/*    // Response*/
/*    read_all(fd, &res, sizeof(res.header));*/
/*    read_all(fd, res.payload, res.header.size);*/
/*    printf("res: %d\n", res.payload[0]);*/
/**/
/*    // RESET message*/
/*    req.header = (RequestHeader) {*/
/*        .type = RESET,*/
/*        .size = 0*/
/*    };*/
/*    write_all(fd, &req, sizeof(req.header));*/
/**/
/*    // Response*/
/*    read_all(fd, &res, sizeof(res.header));*/
/*    read_all(fd, res.payload, res.header.size);*/
/**/
/*    close(fd);*/
/**/
/*    return 0;*/
/*}*/
