# Shenzhen

A hobby x86 operating system written in C.

## Project structure

```
myos/
├── boot/
│   ├── boot.s       # Assembly entry point + multiboot2 header
│   └── linker.ld    # Linker script — memory layout
├── include/
│   └── vga.h        # VGA text driver interface
├── kernel/
│   ├── main.c       # kernel_main() — OS entry point
│   └── vga.c        # VGA 80x25 text mode driver
└── Makefile
```

## Building

### Prerequisites

**Option A — cross-compiler (recommended)**
Install an i686-elf GCC cross-compiler. See:
https://wiki.osdev.org/GCC_Cross-Compiler

```bash
# On Ubuntu/Debian, or build from source
sudo apt install gcc-multilib binutils
```

**Option B — host gcc with -m32**
The Makefile auto-detects and falls back to `-m32` on a native x86-64 machine.

### Build

```bash
make           # builds myos.elf
make iso       # builds myos.iso (needs grub-mkrescue + xorriso)
```

### Run in QEMU

```bash
make run       # QEMU with curses (terminal)
make run-vga   # QEMU with graphical VGA window
```

Or directly:
```bash
qemu-system-i386 -kernel myos.elf
```

## Roadmap

- [x] Multiboot2 boot via GRUB
- [x] VGA text mode (80x25) with color and scrolling
- [ ] GDT (Global Descriptor Table)
- [ ] IDT + interrupt handlers (ISRs/IRQs)
- [ ] PIC remapping
- [ ] Keyboard driver (PS/2)
- [ ] Physical memory manager
- [ ] Virtual memory / paging
- [ ] Heap allocator (kmalloc/kfree)
- [ ] Basic shell
- [ ] Filesystem (FAT32 or custom)
- [ ] User mode (ring 3)
- [ ] ELF loader / processes

## Resources

- https://wiki.osdev.org — the wiki for hobby OS dev
- https://www.gnu.org/software/grub/manual/multiboot2/ — multiboot2 spec
- Intel IA-32 Software Developer's Manual (free PDF from Intel)
