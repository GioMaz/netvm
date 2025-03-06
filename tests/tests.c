#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>

#include "../program.h"
#include "../utils.h"
#include "../vm.h"

#include "tests.h"

int32_t factorial(int32_t n)
{
    int32_t res = 1;
    while (n) res *= n--;
    return res;
}

void check_error(char *error, int testno)
{
    if (error) {
        printf("Test %d failed: %s\n", testno, error);
        exit(1);
    } else {
        printf("Test %d completed\n", testno);
    }
}

int main()
{
    test_exec_1();
    test_exec_2();
    test_exec_3();
    test_exec_4();
}
