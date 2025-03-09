#include "cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

/*
 * Get value of selected register.
 */
static int get_reg_by_num(struct cpu *cpu, int reg_num)
{
    switch (reg_num) {
    case 0:
        return cpu->A;
    case 1:
        return cpu->B;
    case 2:
        return cpu->C;
    case 3:
        return cpu->D;
    default:
        cpu->status = cpuIllegalOperand;
        return -1;
    }
}

/*
 * Modofy add value to selected cpu register. Value can be also negative.
 */
static int modify_reg(struct cpu *cpu, int reg_num, int value, int add)
{
    int32_t *p_reg;
    switch (reg_num) {
    case 0:
        p_reg = &cpu->A;
        break;
    case 1:
        p_reg = &cpu->B;
        break;
    case 2:
        p_reg = &cpu->C;
        break;
    case 3:
        p_reg = &cpu->D;
        break;
    default:
        cpu->status = cpuIllegalOperand;
        return 1;
    }

    if (add) {
        *p_reg += value;

#ifdef BONUS_JMP
        cpu->result = *p_reg;
#endif

    } else {
        *p_reg = value;
    }

    return 0;
}

/*
 * Chceck if insruction of size "len" is whole stored in the memory.
 * 
 * Args:
 *      cpu - emulated cpu structure
 *      len - length of instruction
 * 
 * Returns:
 *      0 if ok, 1 otherwise
 */
static int check_instruction(struct cpu *cpu, int len)
{
    if (&cpu->memory[cpu->instructionPointer + len - 1] <= cpu->stackLimit) {
        return 0;
    }
    cpu->status = cpuInvalidAddress;
    return 1;
}


/*
 *******************
 * CPU INSTUCTIONS
 *******************
 */


/*
 * Do nothing (empty instruction).
 */
static int nop(struct cpu *cpu)
{
    cpu->instructionPointer += 1;
    return 2;
}


/*
 * Stop cpu.
 */
static int halt(struct cpu *cpu)
{
    cpu->status = cpuHalted;
    cpu->instructionPointer += 1;
    return 0;
}


/*
 * Add value from REG to reg A
 * arg1 - REG
 */
static int add(struct cpu *cpu)
{
    int val = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    if (val == -1) {
        return 0;
    }
    cpu->A += val;

#ifdef BONUS_JMP
    cpu->result = cpu->A;
#endif
    return 1;
}


/*
 * Substract value from REG from reg A
 * arg1 - REG
 */
static int sub(struct cpu *cpu)
{
    int val = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    if (val == -1) {
        return 0;
    }
    cpu->A -= val;

#ifdef BONUS_JMP
    cpu->result = cpu->A;
#endif
    return 1;
}


/*
 * Multiply reg A using value from REG
 * arg1 - REG
 */
static int mul(struct cpu *cpu)
{
    int val = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    if (val == -1) {
        return 0;
    }
    cpu->A *= val;

#ifdef BONUS_JMP
    cpu->result = cpu->A;
#endif
    return 1;
}


/*
 * Divide reg A using value from REG
 * arg1 - REG
 */
static int divi(struct cpu *cpu)
{
    int val = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    if (cpu->status != cpuOK) {
        return 0;
    }
    if (val == 0) {
        cpu->status = cpuDivByZero;
        return 0;
    }
    cpu->A /= val;

#ifdef BONUS_JMP
    cpu->result = cpu->A;
#endif
    return 1;
}


/*
 * Inclement REG
 * arg1 - REG
 */
static int inc(struct cpu *cpu)
{
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], 1, 1)) {
        return 0;
    }
    return 1;
}


/*
 * Decrement REG
 * arg1 - REG
 */
static int dec(struct cpu *cpu)
{
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], -1, 1)) {
        return 0;
    }
    return 1;
}


/*
 * If c != 0 jump to instuction in mumory on INDEX
 * arg1 - INDEX
 */
static int loop(struct cpu *cpu)
{
    if (cpu->C != 0) {
        cpu->instructionPointer = cpu->memory[cpu->instructionPointer + 1];
        return 2;
    }
    return 1;
}


/*
 * Set REG to value
 * arg1 - REG
 * arg2 - value
 */
static int movr(struct cpu *cpu)
{
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], cpu->memory[cpu->instructionPointer + 2], 0)) {
        return 0;
    }
    return 1;
}


/*
 * Copy value from top of stack + reg D + offset to REG
 * arg1 - REG
 * arg2 - offset
 */
static int load(struct cpu *cpu)
{
    int *mem_val = &cpu->stackBottom[-cpu->stackSize + cpu->D + cpu->memory[cpu->instructionPointer + 2] + 1];
    if (mem_val <= &cpu->stackBottom[-cpu->stackSize] || mem_val > cpu->stackBottom) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], *(mem_val), 0)) {
        return 0;
    }
    return 1;
}


/*
 * Copy value from REG to top of stack + reg D + offset
 * arg1 - REG
 * arg2 - offset
 */
static int store(struct cpu *cpu)
{
    int *mem_val = &cpu->stackBottom[-cpu->stackSize + cpu->D + cpu->memory[cpu->instructionPointer + 2] + 1];
    if (mem_val <= &cpu->stackBottom[-cpu->stackSize] || mem_val > cpu->stackBottom) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }
    *(mem_val) = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    return 1;
}


/*
 * Load int32 from stdin to REG
 * arg1 - REG
 */
static int in(struct cpu *cpu)
{
    int32_t val;
    if (scanf("%" SCNd32, &val) != 1) {
        cpu->status = cpuIOError;
        return 0;
    }
    if (val == EOF) {
        if (modify_reg(cpu, 2, 0, 0)) {
            return 0;
        }
        if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], -1, 0)) {
            return 0;
        }
    }
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], val, 0)) {
        return 0;
    }
    return 1;
}


/*
 * Load char from stdin to REG
 * arg1 - REG
 */
static int get(struct cpu *cpu)
{
    char val;
    if (scanf("%c", &val) != 1) {
        cpu->status = cpuIOError;
        return 0;
    }
    if (val == EOF) {
        if (modify_reg(cpu, 2, 0, 0)) {
            return 0;
        }
        if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], -1, 0)) {
            return 0;
        }
    }
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], val, 0)) {
        return 0;
    }
    return 1;
}


/*
 * Print int from REG to stdout
 * arg1 - REG
 */
static int out(struct cpu *cpu)
{
    printf("%d", get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]));
    return 1;
}


/*
 * Print char from REG to stdout
 * arg1 - REG
 */
static int put(struct cpu *cpu)
{
    int pom = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    if (pom >= 255 || pom < 0) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }
    printf("%c", pom);
    return 1;
}


/*
 * Swap REG1 and REG2 values
 * arg1 - REG1
 * arg2 - REG2
 */
static int swap(struct cpu *cpu)
{
    int32_t pom1 = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    int32_t pom2 = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 2]);
    if (cpu->status == cpuIllegalOperand) {
        return 0;
    }
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], pom2, 0)) {
        return 0;
    }
    if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 2], pom1, 0)) {
        return 0;
    }
    return 1;
}


/*
 * Add value from REG to top of stack
 * arg1 - REG
 */
static int push(struct cpu *cpu)
{
    if (&cpu->stackBottom[-cpu->stackSize] > cpu->stackLimit) {
        cpu->stackBottom[-cpu->stackSize] = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
        cpu->stackSize++;
        return 1;
    }
    cpu->status = cpuInvalidStackOperation;
    return 0;
}


/*
 * Move value from top of the stack to REG
 * arg1 - REG
 */
static int pop(struct cpu *cpu)
{
    if (cpu->stackSize > 0) {
        int pom = cpu->stackBottom[-cpu->stackSize + 1];
        if (modify_reg(cpu, cpu->memory[cpu->instructionPointer + 1], pom, 0)) {
            return 0;
        }
        cpu->stackSize--;
        return 1;
    }
    cpu->status = cpuInvalidStackOperation;
    return 0;
}

#ifdef BONUS_JMP
/*
 * Compare REG1 - REG2 and result is stored in result register.
 * arg1 - REG1
 * arg1 - REG2
 */
static int cmp(struct cpu *cpu)
{
    int32_t pom1 = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 1]);
    int32_t pom2 = get_reg_by_num(cpu, cpu->memory[cpu->instructionPointer + 2]);
    if (cpu->status != cpuOK) {
        return 0;
    }
    cpu->result = pom1 - pom2;
    return 1;
}

/*
 * Jump to instuction in memory on INDEX
 * arg1 - INDEX
 */
static int jmp(struct cpu *cpu)
{
    cpu->instructionPointer = cpu->memory[cpu->instructionPointer + 1];
    return 2;
}

/*
 * If resul == 0 jump to instuction in memory on INDEX
 * arg1 - INDEX
 */
static int jz(struct cpu *cpu)
{
    if (cpu->result == 0) {
        return jmp(cpu);
    }
    return 1;
}

/*
 * If resul != 0 jump to instuction in memory on INDEX
 * arg1 - INDEX
 */
static int jnz(struct cpu *cpu)
{
    if (cpu->result != 0) {
        return jmp(cpu);
    }
    return 1;
}

/*
 * If resul > 0 jump to instuction in memory on INDEX
 * arg1 - INDEX
 */
static int jgt(struct cpu *cpu)
{
    if (cpu->result > 0) {
        return jmp(cpu);
    }
    return 1;
}

#endif


#ifdef BONUS_CALL
/*
 * Copy index of next instruction to the top of stack.
 * Then jump to instruction index given by INDEX.
 * arg1 - INDEX
 */
static int call(struct cpu *cpu)
{
    if (&cpu->stackBottom[-cpu->stackSize] <= cpu->stackLimit) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }
    cpu->stackBottom[-cpu->stackSize] = cpu->instructionPointer + 2;
    cpu->stackSize++;
    return jmp(cpu);
}


/*
 * Move value from top of the stack to instruction pointer.
 */
static int ret(struct cpu *cpu)
{
    if (cpu->stackSize == 0) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }
    cpu->instructionPointer = cpu->stackBottom[-cpu->stackSize + 1];
    cpu->stackSize--;
    cpu->stackLimit[-cpu->stackSize] = 0;
    return 2;
}
#endif


/*
 ************************
 * Predefined functions
 ************************
 */


/*
 * Allocating 4KiB blocks of memory and load binary instructions into it.
 * In finale realocare memory using size of loaded instructions + size of stack.
 * 
 * Args:
 *      program - handle of file where are stored instructions
 *      stackCapacity - size of stack * sizeof(int32_t)
 *      stackBottom - empty pointer (end of memory pointer will be stored here)
 * 
 * Output:
 *      pointer to begin of allocated memory
 */
int32_t *cpuCreateMemory(FILE *program, size_t stackCapacity, int32_t **stackBottom)
{
    assert(program != NULL);
    assert(stackBottom != NULL);

    int curr_char;
    int32_t *p_temp;
    size_t count = 0;
    size_t chunk_size = 1024 * sizeof(int32_t);
    int32_t codes[4];
    size_t mem_size = chunk_size;
    int32_t *memory = malloc(mem_size);
    if (memory == NULL) {
        fprintf(stderr, "Allocation error!");
        return NULL;
    }
    do {
        curr_char = fgetc(program);
        count++;
        if (count == mem_size) {
            mem_size += chunk_size;
            p_temp = memory;
            memory = realloc(memory, mem_size);
            if (memory == NULL) {
                free(p_temp);
                fprintf(stderr, "Allocation error!");
                return NULL;
            }
        }
        if (count % 4 == 1) {
            codes[0] = curr_char;
        } else if (count % 4 == 2) {
            codes[1] = curr_char;
            codes[1] = codes[1] << 8;
        } else if (count % 4 == 3) {
            codes[2] = curr_char;
            codes[2] = codes[2] << 16;
        } else if (count % 4 == 0) {
            codes[3] = curr_char;
            codes[3] = codes[3] << 24;
            memory[(count / 4) - 1] = codes[0] | codes[1] | codes[2] | codes[3];
        }
    } while (curr_char != EOF);

    if (count % 4 != 1) {
        free(memory);
        fprintf(stderr, "Binary file corrupted!");
        return NULL;
    }
    while (&memory[(count - 1) / 4 - 1] > &memory[((mem_size / 4) - 1) - stackCapacity]) {
        mem_size += chunk_size;
        p_temp = memory;
        memory = realloc(memory, mem_size);
        if (memory == NULL) {
            free(p_temp);
            fprintf(stderr, "Allocation error!");
            return NULL;
        }
    }
    for (int32_t *i = &memory[(count - 1) / 4]; i <= &memory[((mem_size / 4) - 1) - stackCapacity]; i = &i[1]) {
        *i = 0;
    }
    *stackBottom = &memory[(mem_size / 4) - 1];
    return memory;
}


/*
 * Assign values for pointers and set set stack offset and instruction offset to zero.
 * 
 * Args:
 *      cpu - emulated cpu structure
 *      memory - pointer to begin of allocated memory
 *      stackBottom - pointer to end of allocated memory
 *      stackCapacity - size of stack * sizeof(int32_t)
 */
void cpuCreate(struct cpu *cpu, int32_t *memory, int32_t *stackBottom, size_t stackCapacity)
{
    assert(cpu != NULL);
    assert(memory != NULL);
    assert(stackBottom != NULL);

    cpu->memory = memory;
    cpu->stackBottom = stackBottom;
    cpu->stackLimit = &stackBottom[-stackCapacity];
    cpuReset(cpu);
}


/*
 * Free allocated memory and set pointers to NULL value.
 */
void cpuDestroy(struct cpu *cpu)
{
    assert(cpu != NULL);

    free(cpu->memory);
    cpu->memory = NULL;
    cpu->stackBottom = NULL;
    cpu->stackLimit = NULL;
}


/*
 * Set registers to zero, set stack values to zero, set stack offset and instruction offset to zero.
 * 
 * Args:
 *      cpu - emulated cpu structure
 */
void cpuReset(struct cpu *cpu)
{
    assert(cpu != NULL);

    cpu->A = 0;
    cpu->B = 0;
    cpu->C = 0;
    cpu->D = 0;
#ifdef BONUS_JMP
    cpu->result = 0;
#endif
    cpu->status = cpuOK;
    cpu->stackSize = 0;
    cpu->instructionPointer = 0;
    int32_t *i = cpu->stackBottom;
    while (i > cpu->stackLimit) {
        *i = 0;
        i = &i[-1];
    }
}


/*
 * Returns status of emulated cpu.
 */
int cpuStatus(struct cpu *cpu)
{
    assert(cpu != NULL);

    return cpu->status;
}


/*
 * Execute one instruction from memory (+instuction pointer offset).
 * 
 * Args:
 *      cpu - emulated cpu structure
 *      
 * Returns:
 *      0 if error occours, >0 else
 */
int cpuStep(struct cpu *cpu)
{
    assert(cpu != NULL);

    if (cpu->status != cpuOK) {
        return 0;
    }
    if (cpu->memory + cpu->instructionPointer < cpu->memory ||
            cpu->memory + cpu->instructionPointer > cpu->stackLimit) {
        cpu->status = cpuInvalidAddress;
        return 0;
    }
    int inst_lengths[] = { 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 2, 2, 1 };

#ifndef BONUS_CALL
#ifndef BONUS_JMP
    int instruction_count = 18;
    int (*int_foo[])(struct cpu*) = {nop, halt, add, sub, mul, divi, inc, dec, loop, movr, load, store,
                                     in, get, out, put, swap, push, pop};
#endif
#ifdef BONUS_JMP
    int instruction_count = 23;
    int (*int_foo[])(struct cpu*) = {nop, halt, add, sub, mul, divi, inc, dec, loop, movr, load, store,
                                     in, get, out, put, swap, push, pop, cmp, jmp, jz, jnz, jgt};
#endif
#endif
#ifdef BONUS_CALL
    int instruction_count = 25;
    int (*int_foo[])(struct cpu*) = {nop, halt, add, sub, mul, divi, inc, dec, loop, movr, load, store,
                                     in, get, out, put, swap, push, pop, cmp, jmp, jz, jnz, jgt, call, ret};
#endif

    if (cpu->memory[cpu->instructionPointer] >= 0 && cpu->memory[cpu->instructionPointer] <= instruction_count) {
        int inst_len = inst_lengths[cpu->memory[cpu->instructionPointer]];
        if (check_instruction(cpu, inst_len)) {
            return 0;
        }
        int ret_code = (*int_foo[cpu->memory[cpu->instructionPointer]])(cpu);
        if (ret_code == 1) {
            cpu->instructionPointer += inst_len;
        }
        return ret_code;
    }
    cpu->status = cpuIllegalInstruction;
    return 0;
}


/*
 * Call cpuStep function "step" times.
 * 
 * Args:
 *      cpu - emulated cpu structure
 *      steps - number of instructions to do
 * 
 * Returns:
 *      number of successfully completed instructions
 */
int cpuRun(struct cpu *cpu, size_t steps)
{
    assert(cpu != NULL);

    size_t i = 0;
    if (steps <= 0) {
        return 0;
    }
    while (i < steps) {
        cpuStep(cpu);
        i++;
        if (cpu->status == cpuHalted) {
            break;
        }
        if (cpu->status != cpuOK) {
            i = -i;
            break;
        }
    }
    return i;
}


/*
 * Returns value of selected register.
 * 
 * Args:
 *      cpu - emulated cpu structure
 *      reg - name of register (A, B, C, D, S, I)
 * 
 * Returns:
 *      value of register
 */
int32_t cpuPeek(struct cpu *cpu, char reg)
{
    assert(cpu != NULL);

    switch (reg) {
    case 'A':
        return cpu->A;
    case 'B':
        return cpu->B;
    case 'C':
        return cpu->C;
    case 'D':
        return cpu->D;
    case 'S':
        return cpu->stackSize;
    case 'I':
        return cpu->instructionPointer;
    default:
        return 0;
    }
}
