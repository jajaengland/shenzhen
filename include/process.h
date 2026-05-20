#pragma once
#include <stdint.h>
#include <stddef.h>

#define PROCESS_STACK_SIZE  4096    /* 4KB stack per process */
#define MAX_PROCESSES       16

typedef enum {
    PROC_UNUSED  = 0,
    PROC_RUNNING = 1,
    PROC_READY   = 2,
    PROC_BLOCKED = 3,
    PROC_DEAD    = 4,
} proc_state_t;

/* Saved CPU registers for context switching */
typedef struct {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t eip;
} regs_t;

typedef struct process {
    uint32_t     pid;
    proc_state_t state;
    regs_t       regs;
    uint8_t     *stack;          /* bottom of stack */
    uint32_t     stack_top;      /* initial esp */
    void      (*entry)(void);    /* entry function for trampoline */
    char         name[32];
    struct process *next;
} process_t;

void     process_init(void);
process_t *process_create(const char *name, void (*entry)(void));
void     process_exit(void);
void     scheduler_tick(void);
void     yield(void);
uint32_t scheduler_pick(uint32_t cur_esp);
uint32_t preempt_save_and_pick(uint32_t cur_esp);
process_t *process_current(void);
