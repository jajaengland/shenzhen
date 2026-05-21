/*
 * IRQ0 — timer interrupt — preemptive scheduler
 *
 * On entry the CPU has pushed: eip, cs, eflags  (12 bytes)
 * We save everything to current->regs, pick next process,
 * restore from next->regs, and iret.
 */
.section .text
.global irq0_handler
.type irq0_handler, @function
irq0_handler:
    /* save general registers */
    pusha               /* pushes eax,ecx,edx,ebx,esp,ebp,esi,edi */
    push %ds
    push %es

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    call timer_tick

    /* pass frame pointer safely */
    mov %esp, %eax
    push %eax
    call preempt_schedule
    add $4, %esp

    pop %es
    pop %ds
    popa
    iret

/* Trampoline for new processes */
.global process_trampoline
.type process_trampoline, @function
process_trampoline:
    call process_run
    call process_exit
.phang:
    hlt
    jmp .phang

