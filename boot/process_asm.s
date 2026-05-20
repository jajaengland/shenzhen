/*
 * context_switch(regs_t *old, regs_t *new)
 * Simply saves esp into old->esp and loads new->esp.
 * Since we're called via C, ret addresses are on the stacks already.
 * regs_t layout: edi=0, esi=4, ebp=8, esp=12, ebx=16
 */
.section .text
.global context_switch
.type context_switch, @function
context_switch:
    mov 4(%esp), %eax       /* eax = old */
    mov 8(%esp), %edx       /* edx = new */

    /* save callee-saved registers onto old stack */
    push %ebx
    push %esi
    push %edi
    push %ebp

    /* save old esp */
    mov %esp, 12(%eax)

    /* load new esp */
    mov 12(%edx), %esp

    /* restore callee-saved registers from new stack */
    pop %ebp
    pop %edi
    pop %esi
    pop %ebx

    ret
