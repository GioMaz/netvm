#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "program.h"
#include "client.h"
#include "utils.h"
#include "vm.h"

#define CMD_SIZE 64
#define FILENAME_SIZE 64

static void repl_merge_mode(Program *program)
{
    while (1) {
        printf("> ");

        char buffer[INST_SIZE] = {0};

        fgets(buffer, INST_SIZE, stdin);
        if (strcmp(buffer, "done\n") == 0) {
            break;
        }

        Instruction inst = {0};
        bool rv = inst_decode(&inst, buffer);
        if (rv) {
            program_add(program, inst);
        } else {
            fprintf(stderr, "Not a valid instruction\n");
        }
    }
}

static void repl_merge(int fd)
{
    Program program;
    program_init(&program);

    repl_merge_mode(&program);
    if (client_merge_all(fd, &program)) {
        printf("Program merged\n");
    } else {
        fprintf(stderr, "Failed to merge program\n");
    }

    program_deinit(&program);
}

static void repl_insert(int fd, uint64_t start)
{
    Program program;
    program_init(&program);

    repl_merge_mode(&program);
    if (client_insert(fd, &program, start)) {
        printf("Program updated\n");
    } else {
        fprintf(stderr, "Failed to update program\n");
    }

    program_deinit(&program);
}

static void repl_get(int fd)
{
    Program program;
    program_init(&program);

    client_get_all(fd, &program);
    program_print(&program);

    program_deinit(&program);
}

static void repl_exec(int fd)
{
    if (client_exec(fd)) {
        printf("Execution started\n");
    } else {
        fprintf(stderr, "Failed to execute remote program\n");
    }
}

static void repl_delete(int fd, uint32_t start, uint32_t size)
{
    if (client_delete(fd, start, size)) {
        printf("Deleted lines %d to %d\n", start, size);
    } else {
        fprintf(stderr, "Failed to delete lines\n");
    }
}

static void repl_dump(int fd, uint32_t size)
{
    size = (size == 0) ? 16 : size;
    int32_t *memory = (int32_t *)malloc(size*sizeof(int32_t));
    if (client_dump(fd, memory, size)) {
        memory_print(memory, (size_t)size);
    } else {
        fprintf(stderr, "Failed to get memory dump\n");
    }

    free(memory);
}

static void repl_save(int fd, char *filename)
{
    Program program;
    program_init(&program);
    client_get_all(fd, &program);
    if (program_save(filename, &program)) {
        printf("Saved to %s\n", filename);
    } else {
        fprintf(stderr, "Failed to save program to %s\n", filename);
    }

    program_deinit(&program);
}

static void repl_load(int fd, char *filename)
{
    Program program;
    program_init(&program);
    if (program_load(filename, &program)) {
        printf("Loaded from %s.\n", filename);
    } else {
        fprintf(stderr, "Failed to load program from %s\n", filename);
    }

    client_merge_all(fd, &program);
    program_deinit(&program);
}

static void repl_help()
{
    const char *help =
        "Commands: \n"
        "   - merge: write down instructions and then merge them to the server\n"
        "       - this command will enter `merge mode`\n"
        "       - you can write assembly in this format <opcode> <dest> <arg1> <arg2>\n"
        "   - get: get the current state of the server\n"
        "   - exec: execute the current state of the server\n"
        "   - delete <start> <size>: delete <size> instructions starting from <start>\n"
        "   - dump <size>: get memory dump of the first <size> integers in the vm data\n"
        "   - save <filename>: get the remote state and save it to <filename>\n"
        "   - load <filename>: load <filename> state and merge it to the remote state\n"
        "Example usage:\n"
        "   $ merge\n"
        "   > movi 0 69420\n"
        "   > done\n"
        "   $ get\n"
        "   [0x0000]: 9 0 69420 0\n"
        "   $ exec\n"
        "   69420\n";
    printf("%s", help);
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("Failed to create connection socket\n");
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(8080);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    // addr.sin_addr.s_addr = inet_addr("192.168.1.202");
    int rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) {
        die("Failed to connect to server socket\n");
    }

    while (1) {
        printf("$ ");

        char buffer[CMD_SIZE] = {0};
        fgets(buffer, CMD_SIZE, stdin);

        char cmd[CMD_SIZE] = {0};
        int rv = sscanf(buffer, "%s", cmd);

        if (strcmp(cmd, "merge") == 0) {
            repl_merge(fd);
        } else if (strcmp(cmd, "insert") == 0) {
            uint64_t start = 0;
            sscanf(buffer, "%*s %ld", &start);
            repl_insert(fd, start);
        } else if (strcmp(cmd, "get") == 0) {
            repl_get(fd);
        } else if (strcmp(cmd, "exec") == 0) {
            repl_exec(fd);
        } else if (strcmp(cmd, "delete") == 0) {
            uint32_t start = 0, size = 0;
            sscanf(buffer, "%*s %d %d", &start, &size);
            repl_delete(fd, start, size);
        } else if (strcmp(cmd, "dump") == 0) {
            uint32_t size = 0;
            sscanf(buffer, "%*s %d", &size);
            repl_dump(fd, size);
        } else if (strcmp(cmd, "save") == 0) {
            char filename[FILENAME_SIZE] = {0};
            sscanf(buffer, "%*s %s", filename);
            repl_save(fd, filename);
        } else if (strcmp(cmd, "load") == 0) {
            char filename[FILENAME_SIZE] = {0};
            sscanf(buffer, "%*s %s", filename);
            repl_load(fd, filename);
        } else if (strcmp(cmd, "help") == 0) {
            repl_help();
        } else if (strcmp(cmd, "quit") == 0) {
            close(fd);
            break;
        } else if (rv == 1) {
            fprintf(stderr, "Not a valid command.\n");
        }
    }

    return 0;
}
