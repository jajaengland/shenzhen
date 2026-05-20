/* Macro for exceptions WITHOUT error code — push dummy 0 */
.macro ISR_NOERR num
.global isr\num
isr\num:
    cli
    push $0          /* dummy error code */
    push $\num       /* interrupt number */
    jmp isr_common
.endm

/* Macro for exceptions WITH error code — CPU already pushed it */
.macro ISR_ERR num
.global isr\num
isr\num:
    cli
    push $\num
    jmp isr_common
.endm

ISR_NOERR 0    /* divide by zero */
ISR_NOERR 1    /* debug */
ISR_NOERR 2    /* NMI */
ISR_NOERR 3    /* breakpoint */
ISR_NOERR 4    /* overflow */
ISR_NOERR 5    /* bound range exceeded */
ISR_NOERR 6    /* invalid opcode */
ISR_NOERR 7    /* device not available */
ISR_ERR   8    /* double fault */
ISR_NOERR 9    /* coprocessor segment overrun */
ISR_ERR   10   /* invalid TSS */
ISR_ERR   11   /* segment not present */
ISR_ERR   12   /* stack fault */
ISR_ERR   13   /* general protection fault */
ISR_ERR   14   /* page fault */
ISR_NOERR 15
ISR_NOERR 16   /* x87 FPU error */
ISR_ERR   17   /* alignment check */
ISR_NOERR 18   /* machine check */
ISR_NOERR 19   /* SIMD FP exception */
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

.section .text
isr_common:
    pusha
    push %ds
    mov $0x10, %ax
    mov %ax, %ds
    push %esp           /* pass pointer to frame */
    call exception_handler
    add $4, %esp
    pop %ds
    popa
    add $8, %esp        /* skip int_no and err_code */
    iret
