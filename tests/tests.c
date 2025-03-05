#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
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

void test_program_fibonacci()
{
    const int n = 99999;
    int m = n;
    int first = 1, expected = 1;
    while (--m > 1) {
        int tmp = expected + first;
        first = expected;
        expected = tmp;
    }

    Vm vm;
    vm_init(&vm);

    Program program;
    program_init(&program);

    Instruction i0  = { MOVI,    R0, n };
    Instruction i1  = { MOVI,    R1, 1 };
    Instruction i2  = { MOVI,    R2, 1 };
    Instruction i3  = { SUBI,    R0, R0, 1 };
    Instruction i4  = { ADD,     R3, R1, R2 };
    Instruction i5  = { MOV,     R1, R2 };
    Instruction i6  = { MOV,     R2, R3 };
    Instruction i7  = { BEQI,    9, R0, 2 };
    Instruction i8  = { B,       3 };
    Instruction i9  = { MOV,     R0, R2 };
    Instruction i10 = { HALT };

    program_add(&program, i0);
    program_add(&program, i1);
    program_add(&program, i2);
    program_add(&program, i3);
    program_add(&program, i4);
    program_add(&program, i5);
    program_add(&program, i6);
    program_add(&program, i7);
    program_add(&program, i8);
    program_add(&program, i9);
    program_add(&program, i10);

    program_merge(vm.program, program.items, program.size);

    // Assert equality of the two programs
    for (size_t i = 0; i < program.size; i++) {
        Instruction *pi1 = program_fetch(vm.program, i);
        Instruction *pi2 = program_fetch(&program, i);
        assert(inst_eq(pi1, pi2));
    }

    // Run program
    loop(&vm);

    // Assert first element is fibonacci
    assert(vm.memory[0] == expected && "Fibonacci computation result is not correct");
}

int main()
{
    /*test_program_factorial();*/
    /*test_program_fibonacci();*/
    test_exec_1();
    test_exec_2();
    test_exec_3();
    test_exec_4();
}
