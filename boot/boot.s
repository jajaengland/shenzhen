/* Multiboot1 header */
.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    (1 << 1)    /* request memory map */
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot, "a"
.align 4
multiboot_header:
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHECKSUM

/* Stack */
.section .bss, "aw", @nobits
.align 16
stack_bottom:
    .skip 16384
stack_top:

/* Entry point */
.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    push %ebx           /* multiboot info pointer in ebx */
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang
