# MyOS build system
# Targets a bare-metal i686 ELF kernel

# --- Toolchain ---
# Use cross-compiler if available, fall back to host gcc with -m32
CROSS := i686-elf-
CC    := $(shell which $(CROSS)gcc 2>/dev/null || echo gcc)
AS    := $(shell which $(CROSS)as  2>/dev/null || echo as)
LD    := $(shell which $(CROSS)ld  2>/dev/null || echo ld)

# Cross-compile flags
CFLAGS  := -m32 -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
           -Iinclude -fno-stack-protector -fno-pic \
           -mno-red-zone
ASFLAGS := --32
LDFLAGS := -m elf_i386 -T boot/linker.ld -nostdlib

# --- Sources ---
C_SRCS  := kernel/main.c kernel/vga.c kernel/gdt.c kernel/idt.c kernel/kprintf.c kernel/pmm.c kernel/heap.c kernel/process.c kernel/timer.c kernel/exceptions.c
AS_SRCS := boot/boot.s boot/gdt_flush.s boot/idt_asm.s boot/process_asm.s boot/timer_asm.s boot/exceptions_asm.s

C_OBJS  := $(C_SRCS:.c=.o)
AS_OBJS := $(AS_SRCS:.s=.o)
OBJS    := $(AS_OBJS) $(C_OBJS)

KERNEL  := myos.elf
ISO     := myos.iso

# --- Rules ---
.PHONY: all clean iso run

all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "  LINK  $@"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "  CC    $<"

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<
	@echo "  AS    $<"

# Build a bootable ISO (requires grub-mkrescue and xorriso)
iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo 'set timeout=0'                          >  iso/boot/grub/grub.cfg
	echo 'set default=0'                          >> iso/boot/grub/grub.cfg
	echo 'menuentry "MyOS" {'                     >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/myos.elf'          >> iso/boot/grub/grub.cfg
	echo '    boot'                               >> iso/boot/grub/grub.cfg
	echo '}'                                      >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso 2>/dev/null
	@echo "  ISO   $(ISO)"

# Run in QEMU (no ISO needed — load ELF directly via -kernel)
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -display curses

# Run with VGA output in a window
run-vga: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -rf iso
