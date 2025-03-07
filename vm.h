#ifndef VM_H
#define VM_H

#include "program.h"

typedef enum {
    OK,
    MEMORY_OVERFLOW,
    MALFORMED_INSTRUCTION,
    DIVISION_BY_ZERO
} InstResult;

#define RES_STRING(res) #res
static const char *res_names[] = {
    RES_STRING(OK),
    RES_STRING(MEMORY_OVERFLOW),
    RES_STRING(MALFORMED_INSTRUCTION),
    RES_STRING(DIVISION_BY_ZERO)
};
#undef RES_STRING

// ABI
#define R0 0 // vm->memory[R0] tmp0 (return value)
#define R1 1 // vm->memory[R1] tmp1
#define R2 2 // vm->memory[R2] tmp2
#define R3 3 // vm->memory[R3] tmp3
#define PC 4 // vm->memory[PC] program counter
#define LR 5 // vm->memory[LR] link register
#define BP 6 // vm->memory[BP] base pointer
#define SP 7 // vm->memory[SP] stack pointer
#define SB 8 // vm->memory[SB] stack base

#define CONTEXT_SIZE 8
#define MEMORY_SIZE 1024
#define TIMER_LIMIT 0xffff

typedef struct {
    Program *program;
    int32_t memory[MEMORY_SIZE];
    uint16_t timer;
} Vm;

typedef enum {
    LR_CONTEXT_CHANGED,
    LR_TIME_EXCEEDED,
    LR_MALFORMED_INSTRUCTION,
    LR_SUCCESS,
} LoopResult;

// interpreter
void vm_init(Vm *vm);
void vm_deinit(Vm *vm);
void vm_setreg(Vm *vm);

// Memory
void memory_dump(Vm *vm);
void memory_print(int *memory, size_t size);

// Fetch-execute loop
LoopResult loop(Vm *vm);
bool loop_dbg(Vm *vm);
Instruction *fetch(Vm *vm);
InstResult execute(Vm *vm, Instruction *inst);

// Arithmetic instructions
InstResult add(Vm *vm, int dest, int arg1, int arg2);
InstResult addi(Vm *vm, int dest, int arg1, int arg2);
InstResult sub(Vm *vm, int dest, int arg1, int arg2);
InstResult subi(Vm *vm, int dest, int arg1, int arg2);
InstResult mul(Vm *vm, int dest, int arg1, int arg2);
InstResult muli(Vm *vm, int dest, int arg1, int arg2);
// Renamed to ddiv to avoid conflict with stdlib's div_t div(int, int)
InstResult ddiv(Vm *vm, int dest, int arg1, int arg2);
InstResult divi(Vm *vm, int dest, int arg1, int arg2);

// Stack manipulation instructions
InstResult movi(Vm *vm, int dest, int arg1);
InstResult push(Vm *vm, int dest);
InstResult pushi(Vm *vm, int dest);
InstResult pop(Vm *vm, int dest);
InstResult sallo(Vm *vm, int dest);
InstResult sfree(Vm *vm, int dest);
InstResult hallo(Vm *vm, int dest);
InstResult hfree(Vm *vm, int dest);

// Control instruction
InstResult beq(Vm *vm, int dest, int arg1, int arg2);
InstResult beqi(Vm *vm, int dest, int arg1, int arg2);
InstResult bne(Vm *vm, int dest, int arg1, int arg2);
InstResult bnei(Vm *vm, int dest, int arg1, int arg2);
InstResult bge(Vm *vm, int dest, int arg1, int arg2);
InstResult bgei(Vm *vm, int dest, int arg1, int arg2);

InstResult ret(Vm *vm, int dest);
InstResult reti(Vm *vm, int dest);
InstResult halt(Vm *vm);

#endif
