.section .text
.global gdt_flush
.type gdt_flush, @function
gdt_flush:
    mov 4(%esp), %eax       /* pointer to gdt_ptr struct */
    lgdt (%eax)             /* load the GDT */

    mov $0x10, %ax          /* 0x10 = kernel data segment (index 2) */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    ljmp $0x08, $flush2     /* far jump to reload CS with kernel code segment */
flush2:
    ret
