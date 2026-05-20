.section .text
.global irq0_handler
.type irq0_handler, @function
irq0_handler:
    /* CPU pushed: eip, cs, eflags at [esp], [esp+4], [esp+8] */
    /* Save all registers */
    pusha                   /* esp after: points to saved edi */
    push %ds
    push %es
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es

    call timer_tick

    /* Check if we should switch.
       Pass current esp (points to our full IRQ frame) */
    push %esp
    call preempt_save_and_pick
    add $4, %esp

    test %eax, %eax
    jz .no_preempt

    /* Switch to new process stack.
       The new process stack has cooperative format: [ebx,esi,edi,ebp,ret_addr]
       We need to convert it to IRQ format so iret works.
       Get the ret_addr from the cooperative stack and build an iret frame */
    mov %eax, %esp          /* switch to new process cooperative stack */
    /* cooperative stack: esp -> [ebx][esi][edi][ebp][ret_addr] */
    pop %ebx
    pop %esi
    pop %edi
    pop %ebp
    pop %eax                /* eax = ret_addr (where process runs) */

    /* now build a fake IRQ frame for iret:
       push eflags (IF=1), cs, eip */
    pushf
    orl $0x200, (%esp)      /* set IF flag */
    push $0x08              /* cs */
    push %eax               /* eip = process entry */
    /* push dummy ds/es and pusha frame so our pop ds/es/popa work */
    push $0x10              /* es */
    push $0x10              /* ds */
    pusha
    /* fall through to no_preempt which will pop and iret */

.no_preempt:
    pop %es
    pop %ds
    popa
    iret

/* Trampoline — new processes start here */
.global process_trampoline
.type process_trampoline, @function
process_trampoline:
    call process_run
    call process_exit
.phang:
    hlt
    jmp .phang

