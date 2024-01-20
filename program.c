#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "program.h"

bool program_init(Program *program)
{
    program->v = (Instruction *)malloc(sizeof(Instruction));
    if (program->v == NULL)
        return false;

    program->capacity = 1;
    program->size = 0;

    return true;
}

bool program_deinit(Program *program)
{
    if (program == NULL)
        return false;

    if (program->v == NULL) {
        return false;
    } else {
        free(program->v);
    }

    return true;
}

Instruction *program_data(Program *program)
{
    return program->v;
}

size_t program_capacity(Program *program)
{
    return program->capacity;
}

size_t program_size(Program *program)
{
    return program->size;
}

bool program_resize(Program *program, size_t capacity_new)
{
    Instruction *v_new = realloc(program->v, capacity_new * sizeof(Instruction));
    if (v_new == NULL) {
        printf("Memory reallocation failed.\n");
        return false;
    }

    program->v = v_new;
    program->capacity = capacity_new;

    return true;
}

bool program_inc_capacity(Program *program)
{
    size_t capacity_new = 2 * program->capacity;
    return program_resize(program, capacity_new);
}

bool program_clear(Program *program)
{
    program->size = 0;
    return true;
}

bool program_copy(Program *program, Program *src)
{
    bool rv = program_resize(program, src->capacity);
    if (!rv) {
        printf("Failed resize during program copy.\n");
        return false;
    }

    memcpy(program->v, src->v, src->size * sizeof(Instruction));

    program->size = src->size;
    program->capacity = src->capacity;

    return true;
}

bool program_merge(Program *program, Instruction *src, size_t size)
{
    bool rv = program_resize(program, program->capacity + size);
    if (!rv) {
        printf("Failed resize during program merge.\n");
        return false;
    }

    Instruction *dst = program->v + program->size;

    memcpy(dst, src, size * sizeof(Instruction));
    program->size += size;

    return true;
}

bool program_split(Program *program, Instruction *dst, size_t size)
{
    memcpy(dst, program->v, size * sizeof(Instruction));
    size_t size_new = program->size - size;
    program->size = size_new;
    if (size_new) {
        memmove(program->v, program->v + size, size_new * sizeof(Instruction));
    }
    return true;
}

bool program_delete(Program *program, size_t start, size_t end)
{
    if (start > end) {
        printf("Start must be greater than end.\n");
        return false;
    }

    if (start < 0) {
        printf("Start must be positive.\n");
        return false;
    }

    if (end > program->size) {
        printf("End must be less than program size.\n");
        return false;
    }

    // Include end
    end++;

    size_t size = end - start;
    memmove(program->v + start, program->v + end, size * sizeof(Instruction));

    program->size -= size;

    return true;
}

// Pointer to program cause we may need to
// re-allocate the Instruction vector;
bool program_add(Program *program, Instruction inst)
{
    if (program->size == program->capacity) {
        if (!program_inc_capacity(program)) {
            printf("Failed to increase capacity.\n");
            return false;
        }
    }

    program->v[program->size] = inst;
    program->size++;

    return true;
}

Instruction *program_fetch(Program *program, size_t index)
{
    return &program->v[index];
}

void program_print(Program *program)
{
    Instruction *v = program->v;

    for (size_t i = 0; i < program->size; i++) {
        inst_print(v[i], i);
    }
}

void inst_print(Instruction inst, size_t index)
{
    printf("[0x%.4zx]: %i %i %i %i\n", index, inst.code,
        inst.dest, inst.arg1, inst.arg2);
}
