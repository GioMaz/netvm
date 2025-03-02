#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "program.h"

// Interpreter
void vm_init(Vm *vm)
{
    Program *program = (Program *)malloc(sizeof(Program));
    program_init(program);
    vm->program = program;

    // Setup registers
    vm_setreg(vm);
}

void vm_deinit(Vm *vm)
{
    program_deinit(vm->program);
    free(vm->program);
}

void vm_setreg(Vm *vm)
{
    vm->memory[R0] = 0;
    vm->memory[R1] = 0;
    vm->memory[R2] = 0;
    vm->memory[R3] = 0;
    vm->memory[PC] = 0;
    vm->memory[BP] = SB;
    vm->memory[SP] = SB;
}

// Memory
void MEMORY_dump(Vm *vm)
{
    int limit = 16;
    for (int i = 0; i < limit && i < MEMORY_SIZE; i++) {
        printf("[%i]:\t%i\n", i, vm->memory[i]);
    }
}

// Fetch-execute loop
void loop(Vm *vm)
{
    Instruction *inst;
    while (vm->memory[PC] < program_size(vm->program)) {
        // Fetch instruction
        inst = fetch(vm);

        // Increment program counter
        vm->memory[PC]++;

        // Execute instruction
        InstResult res = execute(vm, inst);

        // Handle result
        if (res != OK) {
            printf("Error: %s at instruction %d\n", res_names[res], vm->memory[PC]);
            break;
        }
    }
}

// Fetch-execute loop nonblocking
bool loopn(Vm *vm)
{
    Instruction *inst;
    size_t count = 0;
    while (1) {
        // Fetch instruction
        inst = fetch(vm);

        // Increment program counter
        vm->memory[PC]++;
        count++;

        // Execute instruction
        InstResult res = execute(vm, inst);

        // Handle result
        if (res != OK) {
            printf("Error: %s at instruction %d\n", res_names[res], vm->memory[PC]);
            break;
        }

        // Check if program finished
        if (vm->memory[PC] >= program_size(vm->program)) {
            return true;
        }

        // Check if execution exceeded context size
        if (count >= CONTEXT_SIZE) {
            return false;
        }
    }

    return true;
}

bool loop_dbg(Vm *vm)
{
    // Fetch instruction
    Instruction *inst = fetch(vm);

    // Increment program counter
    vm->memory[PC]++;

    // Execute instruction
    InstResult res = execute(vm, inst);

    // Handle result
    if (res != OK) {
        printf("Error: %s at instruction %d\n", res_names[res], vm->memory[PC]);
    }

    // Check if program finished
    if (vm->memory[PC] >= program_size(vm->program)) {
        return true;
    }

    printf("Program dump:\n");
    Instruction *items = vm->program->items;
    for (size_t i = 0; i < vm->program->size; i++) {
        if (vm->memory[PC] == i)
            inst_print_curr(items[i], i);
        else
            inst_print(items[i], i);
    }

    printf("Memory dump:\n");
    MEMORY_dump(vm);

    return true;
}

Instruction *fetch(Vm *vm)
{
    return program_fetch(vm->program, vm->memory[PC]);
}

// Pointer to interpreter cause we need
// to increase the program counter
InstResult execute(Vm *vm, Instruction *inst)
{
    int dest = inst->dest;
    int arg1 = inst->arg1;
    int arg2 = inst->arg2;

    InstResult res;
    switch (inst->code) {
    case ADD:   res = add(vm, dest, arg1, arg2); break;
    case ADDI:  res = addi(vm, dest, arg1, arg2); break;
    case SUB:   res = sub(vm, dest, arg1, arg2); break;
    case SUBI:  res = subi(vm, dest, arg1, arg2); break;
    case MUL:   res = mul(vm, dest, arg1, arg2); break;
    case MULI:  res = muli(vm, dest, arg1, arg2); break;
    case DIV:   res = ddiv(vm, dest, arg1, arg2); break;
    case DIVI:  res = divi(vm, dest, arg1, arg2); break;
    case MOV:   res = addi(vm, dest, arg1, 0); break;
    case MOVI:  res = movi(vm, dest, arg1); break;
    case PUSH:  res = push(vm, dest); break;
    case PUSHI: res = pushi(vm, dest); break;
    case POP:   res = pop(vm, dest); break;
    case B:     res = beq(vm, dest, 0, 0); break;
    case BEQ:   res = beq(vm, dest, arg1, arg2); break;
    case BEQI:  res = beqi(vm, dest, arg1, arg2); break;
    case BNE:   res = bne(vm, dest, arg1, arg2); break;
    case BNEI:  res = bnei(vm, dest, arg1, arg2); break;
    case BGE:   res = bge(vm, dest, arg1, arg2); break;
    case BGEI:  res = bgei(vm, dest, arg1, arg2); break;
    case BLEI:  res = bgei(vm, dest, arg2, arg1); break;
    case RET:   res = ret(vm, dest); break;
    case RETI:  res = reti(vm, dest); break;
    case HALT:  res = halt(vm); break;
    default:
        res = MALFORMED_INSTRUCTION;
        break;
    }

    return res;
}

#define CHECK_MEMORY_BOUNDS(arg) \
        arg >= 0 && arg < MEMORY_SIZE

#define CHECK_MEMORY_BOUNDS_2(arg1, arg2) \
        arg1 >= 0 && arg1 < MEMORY_SIZE \
     && arg2 >= 0 && arg2 < MEMORY_SIZE

#define CHECK_MEMORY_BOUNDS_3(arg1, arg2, arg3) \
        arg1 >= 0 && arg1 < MEMORY_SIZE \
     && arg2 >= 0 && arg2 < MEMORY_SIZE \
     && arg3 >= 0 && arg3 < MEMORY_SIZE

// Instructions
InstResult add(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_3(dest, arg1, arg2)) {
        vm->memory[dest] = vm->memory[arg1] + vm->memory[arg2];
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult addi(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_2(dest, arg1)) {
        vm->memory[dest] = vm->memory[arg1] + arg2;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult sub(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_3(dest, arg1, arg2)) {
        vm->memory[dest] = vm->memory[arg1] - vm->memory[arg2];
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult subi(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_2(dest, arg1)) {
        vm->memory[dest] = vm->memory[arg1] - arg2;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult mul(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_3(dest, arg1, arg2)) {
        vm->memory[dest] = vm->memory[arg1] * vm->memory[arg2];
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult muli(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_2(dest, arg1)) {
        vm->memory[dest] = vm->memory[arg1] * arg2;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult ddiv(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_3(dest, arg1, arg2)) {
        int den = vm->memory[arg2];
        if (den == 0)
            return DIVISION_BY_ZERO;

        vm->memory[dest] = vm->memory[arg1] / den;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult divi(Vm *vm, int dest, int arg1, int arg2)
{
    if (arg2 == 0)
        return DIVISION_BY_ZERO;

    if (CHECK_MEMORY_BOUNDS_2(dest, arg1)) {
        vm->memory[dest] = vm->memory[arg1] / arg2;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult movi(Vm *vm, int dest, int arg1)
{
    if (CHECK_MEMORY_BOUNDS(dest)) {
        vm->memory[dest] = arg1;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult push(Vm *vm, int dest)
{
    if (CHECK_MEMORY_BOUNDS_2(vm->memory[SP], dest)) {
        vm->memory[vm->memory[SP]] = vm->memory[dest];
        vm->memory[SP]++;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult pushi(Vm *vm, int dest)
{
    if (CHECK_MEMORY_BOUNDS(vm->memory[SP])) {
        vm->memory[vm->memory[SP]] = dest;
        vm->memory[SP]++;
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult pop(Vm *vm, int dest)
{
    if (CHECK_MEMORY_BOUNDS(dest)
        && vm->memory[SP] > 0) {
        vm->memory[SP]--;
        vm->memory[dest] = vm->memory[vm->memory[SP]];
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult beq(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_2(arg1, arg2)
        && dest < program_size(vm->program)) {
        if (arg1 == arg2)
            vm->memory[PC] = dest;
        else if (vm->memory[arg1] == vm->memory[arg2])
            vm->memory[PC] = dest;

        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult beqi(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS(arg1)
        && dest < program_size(vm->program)) {
        if (vm->memory[arg1] == arg2)
            vm->memory[PC] = dest;

        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult bne(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_2(arg1, arg2)
        && dest < program_size(vm->program)) {
        if (vm->memory[arg1] != vm->memory[arg2])
            vm->memory[PC] = dest;

        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult bnei(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS(arg1)
        && dest < program_size(vm->program)) {
        if (vm->memory[arg1] != arg2)
            vm->memory[PC] = dest;

        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult bge(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS_2(arg1, arg2)
        && dest < program_size(vm->program)) {
        if (vm->memory[arg1] >= vm->memory[arg2])
            vm->memory[PC] = dest;

        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult bgei(Vm *vm, int dest, int arg1, int arg2)
{
    if (CHECK_MEMORY_BOUNDS(arg1)
        && dest < program_size(vm->program)) {
        if (vm->memory[arg1] >= arg2)
            vm->memory[PC] = dest;

        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult ret(Vm *vm, int dest)
{
    if (CHECK_MEMORY_BOUNDS(dest)) {
        vm->memory[0] = vm->memory[dest];
        return OK;
    }

    return MEMORY_OVERFLOW;
}

InstResult reti(Vm *vm, int dest)
{
    vm->memory[0] = dest;
    return OK;
}

InstResult halt(Vm *vm)
{
    vm->memory[PC] = program_size(vm->program);
    return OK;
}

#undef CHECK_MEMORY_BOUNDS
#undef CHECK_MEMORY_BOUNDS_2
#undef CHECK_MEMORY_BOUNDS_3
