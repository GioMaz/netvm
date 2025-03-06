#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "../program.h"
#include "../server.h"
#include "../client.h"
#include "tests.h"

void test_exec_1()
{
    const int32_t n = 5;
    int32_t expected = factorial(n);

    int pid = fork();
    if (pid) {
        usleep(1000);
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

        Instruction i0 = { MOVI,    R1, n };
        Instruction i1 = { MOV,     R0, R1 };
        Instruction i2 = { SUBI,    R1, R1, 1 };
        Instruction i3 = { BEQI,    6, R1, 1 };
        Instruction i4 = { MUL,     R0, R0, R1 };
        Instruction i5 = { B,       2 };
        Instruction i6 = { HALT };

        program_add(&program_1, i0);
        program_add(&program_1, i1);
        program_add(&program_1, i2);
        program_add(&program_1, i3);
        program_add(&program_1, i4);
        program_add(&program_1, i5);
        program_add(&program_1, i6);

        program_clone(&program_2, &program_1);

        client_merge_all(fd, &program_1);
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
        int32_t memory;
        client_dump(fd, &memory, 1);

        if (memory != expected) {
            error = "Expected factorial calculation does not match";
        }

        // Clean
        close(fd);
        kill(pid, SIGQUIT);
        program_deinit(&program_1);
        program_deinit(&program_2);

        // Check error
        check_error(error, 1);
    } else {
        freopen("/dev/null", "w", stdout);
        start_server(PORT);
    }
}
