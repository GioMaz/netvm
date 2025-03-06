#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "../program.h"
#include "../server.h"
#include "../client.h"
#include "tests.h"

static char *exec_program_1()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(PORT);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    Program program_1;
    program_init(&program_1);

    Instruction i0 = { B,   0 };
    Instruction i1 = { HALT };

    program_add(&program_1, i0);
    program_add(&program_1, i1);

    client_merge_all(fd, &program_1);
    client_exec(fd);

    // Clean
    program_deinit(&program_1);
    close(fd);

    return NULL;
}

static char *exec_program_2()
{
    const int32_t n = 5;
    int32_t expected = factorial(n);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(PORT);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    Program program_2;
    program_init(&program_2);

    Instruction i0 = { MOVI,    R1, n };
    Instruction i1 = { MOV,     R0, R1 };
    Instruction i2 = { SUBI,    R1, R1, 1 };
    Instruction i3 = { BEQI,    6, R1, 1 };
    Instruction i4 = { MUL,     R0, R0, R1 };
    Instruction i5 = { B,       2 };
    Instruction i6 = { HALT };

    program_add(&program_2, i0);
    program_add(&program_2, i1);
    program_add(&program_2, i2);
    program_add(&program_2, i3);
    program_add(&program_2, i4);
    program_add(&program_2, i5);
    program_add(&program_2, i6);

    client_merge_all(fd, &program_2);

    client_exec(fd);
    int32_t memory;
    client_dump(fd, &memory, 1);

    char *error = NULL;
    if (memory != expected)
        error = "Expected factorial calculation does not match";

    // Clean
    program_deinit(&program_2);
    close(fd);

    return error;
}

void test_exec_4()
{
    int pid = fork();
    if (pid) {
        usleep(1000);

        exec_program_1();

        char *error = NULL;
        error = exec_program_2();

        // Clean
        kill(pid, SIGQUIT);

        // Check error
        check_error(error, 4);
    } else {
        freopen("/dev/null", "w", stdout);
        start_server(PORT);
    }
}
