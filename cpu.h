#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>


/* Represents a 32-bit processor */
#ifndef CPU_H
#define CPU_H

enum cpuStatus
{
    cpuOK,
    cpuHalted,
    cpuIllegalInstruction,
    cpuIllegalOperand,
    cpuInvalidAddress,
    cpuInvalidStackOperation,
    cpuDivByZero,
    cpuIOError
};

/*
 * Emulated cpu structure.
 */
struct cpu
{
    int32_t A;
    int32_t B;
    int32_t C;
    int32_t D;
    enum cpuStatus status;
    int32_t stackSize;
    int32_t instructionPointer;
    int32_t *memory;
    int *stackBottom;
    int *stackLimit;

#ifdef BONUS_JMP
    int32_t result;
#endif
};

/*
 * Allocating 4KiB blocks of memory and load binary instructions into it.
 * In finale realocare memory using size of loaded instructions + size of stack.
 */
int32_t *cpuCreateMemory(FILE *program, size_t stackCapacity, int32_t **stackBottom);

/*
 * Assign values for pointers and set set stack offset and instruction offset to zero.
 */
void cpuCreate(struct cpu *cpu, int32_t *memory, int32_t *stackBottom, size_t stackCapacity);

/*
 * Free allocated memory and set pointers to NULL value.
 */
void cpuDestroy(struct cpu *cpu);

/*
 * Set registers to zero, set stack values to zero, set stack offset and instruction offset to zero.
 */
void cpuReset(struct cpu *cpu);

/*
 * Returns status of emulated cpu.
 */
int cpuStatus(struct cpu *cpu);

/*
 * Execute one instruction from memory (+instuction pointer offset).
 */
int cpuStep(struct cpu *cpu);

/*
 * Call cpuStep function "step" times.
 */
int cpuRun(struct cpu *cpu, size_t steps);

/*
 * Returns value of selected register.
 */
int32_t cpuPeek(struct cpu *cpu, char reg);

#endif
