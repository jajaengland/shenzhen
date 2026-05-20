/* Linker script for i686 MyOS kernel */
/* GRUB loads us at 1 MiB (0x100000) */

ENTRY(_start)

SECTIONS {
    . = 1M;

    /* Multiboot header must be in the first 32 KiB of the file */
    .multiboot ALIGN(4K) : {
        *(.multiboot)
    }

    .text ALIGN(4K) : {
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;
}
