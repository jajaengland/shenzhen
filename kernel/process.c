#include "process.h"
#include "heap.h"
#include "kprintf.h"
#include "vga.h"
#include "io.h"

process_t processes[MAX_PROCESSES];
static process_t *current  = 0;
static process_t *run_queue = 0;   /* head of circular ready list */
static uint32_t   next_pid = 1;

/* Defined in process_asm.s */
extern void context_switch(regs_t *old, regs_t *new_regs);

static void kstrncpy(char *dst, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* Add process to end of circular run queue */
static void queue_add(process_t *p) {
    if (!run_queue) {
        run_queue = p;
        p->next   = p;
        return;
    }
    /* find tail */
    process_t *t = run_queue;
    while (t->next != run_queue) t = t->next;
    t->next = p;
    p->next = run_queue;
}

static void queue_remove(process_t *p) {
    if (!run_queue) return;
    if (run_queue == p && p->next == p) { run_queue = 0; return; }
    process_t *t = run_queue;
    while (t->next != p) t = t->next;
    t->next = p->next;
    if (run_queue == p) run_queue = p->next;
}

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        processes[i].state = PROC_UNUSED;

    /* create the idle/kernel process — represents the current execution context */
    process_t *idle = &processes[0];
    idle->pid   = 0;
    idle->state = PROC_RUNNING;
    idle->next  = idle;
    kstrncpy(idle->name, "kernel", 32);
    current   = idle;
    run_queue = idle;

    kprintf("Processes: initialized (max %d)\n", MAX_PROCESSES);
}

process_t *process_create(const char *name, void (*entry)(void)) {
    /* find a free slot */
    process_t *p = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) { p = &processes[i]; break; }
    }
    if (!p) { kprintf("process_create: no free slots\n"); return 0; }

    /* allocate stack */
    p->stack = (uint8_t *)kmalloc(PROCESS_STACK_SIZE);
    if (!p->stack) { kprintf("process_create: no memory\n"); return 0; }

    /* set up stack for cooperative context_switch:
       context_switch pops: ebp, edi, esi, ebx, then ret */
    uint32_t *sp = (uint32_t *)(p->stack + PROCESS_STACK_SIZE);
    extern void process_trampoline(void);
    *--sp = (uint32_t)process_trampoline;  /* ret addr */
    *--sp = 0;                              /* ebp */
    *--sp = 0;                              /* edi */
    *--sp = 0;                              /* esi */
    *--sp = 0;                              /* ebx */

    p->pid       = next_pid++;
    p->state     = PROC_READY;
    p->stack_top = (uint32_t)sp;
    p->entry     = entry;
    kstrncpy(p->name, name, 32);
    p->regs.esp  = (uint32_t)sp;

    /* set eip to trampoline — preempt_schedule will restore this into IRQ frame */
    extern void process_trampoline(void);
    p->regs.eip = (uint32_t)process_trampoline;
    p->regs.eax = p->regs.ebx = p->regs.ecx = p->regs.edx = 0;
    p->regs.esi = p->regs.edi = p->regs.ebp = 0;

    queue_add(p);
    kprintf("process: created '%s' (pid %d)\n", p->name, p->pid);
    return p;
}

/* Called by process_trampoline — runs the process entry function */
void process_run(void) {
    if (current->entry)
        current->entry();
}

void process_exit(void) {
    kprintf("process: '%s' (pid %d) exited\n", current->name, current->pid);
    queue_remove(current);
    uint8_t *stack = current->stack;
    process_t *dying = current;
    dying->state = PROC_UNUSED;
    dying->stack = 0;
    dying->next  = 0;

    /* find kernel process */
    process_t *kern = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid == 0 && processes[i].state != PROC_UNUSED) {
            kern = &processes[i];
            break;
        }
    }

    current = kern ? kern : &processes[0];
    current->state = PROC_RUNNING;
    kfree(stack);

    /* restore kernel's cooperative stack directly */
    __asm__ volatile (
        "mov %0, %%esp\n\t"
        "pop %%ebp\n\t"
        "pop %%edi\n\t"
        "pop %%esi\n\t"
        "pop %%ebx\n\t"
        "ret"
        :: "r"(current->regs.esp)
        : "memory"
    );
    while(1);
}

process_t *process_current(void) { return current; }

/* IRQ frame layout after pusha + push ds/es */
typedef struct {
    uint32_t es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
} irq_frame_t;

/* Called from IRQ0 with frame pointer.
   Stack when called (from inside preempt_schedule):
   [ret_addr]   <- esp in function
   [esp_before_push] <- the arg we pushed = address of [es] slot
   So frame = (irq_frame_t*)arg, where arg is what esp was before push %esp
   Before push %esp: stack was [es][ds][edi]...[eflags]
   So arg = address of es slot = frame base */
void preempt_schedule(void *raw_frame) {
    (void)raw_frame;
    /* preemptive scheduling disabled — use cooperative yield() instead */
}
/* unused — kept for compatibility */
uint32_t scheduler_pick(uint32_t cur_esp) { (void)cur_esp; return 0; }

void yield(void) {
    scheduler_tick();
}

void scheduler_tick(void) {
    if (!run_queue) return;

    process_t *next = current->next;
    int loops = 0;
    while (next != current) {
        if (next->state == PROC_READY || next->state == PROC_RUNNING) break;
        next = next->next;
        if (++loops > MAX_PROCESSES) return;
    }
    if (next == current) return;

    process_t *old = current;
    current = next;
    current->state = PROC_RUNNING;
    if (old->state == PROC_RUNNING) old->state = PROC_READY;

    context_switch(&old->regs, &current->regs);
}
