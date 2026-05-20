#include "exceptions.h"
#include "idt.h"
#include "vga.h"
#include "kprintf.h"

static const char *exception_names[] = {
    "Division By Zero",       "Debug",
    "Non-Maskable Interrupt", "Breakpoint",
    "Overflow",               "Bound Range Exceeded",
    "Invalid Opcode",         "Device Not Available",
    "Double Fault",           "Coprocessor Segment Overrun",
    "Invalid TSS",            "Segment Not Present",
    "Stack Fault",            "General Protection Fault",
    "Page Fault",             "Reserved",
    "x87 FPU Error",          "Alignment Check",
    "Machine Check",          "SIMD FP Exception",
    "Reserved","Reserved","Reserved","Reserved",
    "Reserved","Reserved","Reserved","Reserved",
    "Reserved","Reserved","Reserved","Reserved",
};

/* defined in exceptions_asm.s */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

void exceptions_init(void) {
    void (*isrs[])(void) = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    };
    for (int i = 0; i < 32; i++)
        idt_set_gate(i, (uint32_t)isrs[i], 0x08, 0x8E);
}

void exception_handler(int_frame_t *frame) {
    /* paint screen red */
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_clear();

    vga_println("*** KERNEL PANIC ***");
    vga_println("");

    if (frame->int_no < 32)
        kprintf("Exception %d: %s\n", frame->int_no, exception_names[frame->int_no]);
    else
        kprintf("Exception %d\n", frame->int_no);

    kprintf("Error code: 0x%x\n", frame->err_code);
    vga_println("");

    kprintf("EIP=0x%x  CS=0x%x  EFLAGS=0x%x\n",
            frame->eip, frame->cs, frame->eflags);
    kprintf("EAX=0x%x  EBX=0x%x  ECX=0x%x  EDX=0x%x\n",
            frame->eax, frame->ebx, frame->ecx, frame->edx);
    kprintf("ESI=0x%x  EDI=0x%x  EBP=0x%x  ESP=0x%x\n",
            frame->esi, frame->edi, frame->ebp, frame->esp);
    kprintf("DS=0x%x\n", frame->ds);
    vga_println("");
    vga_set_color(VGA_YELLOW, VGA_RED);
    vga_println("System halted. Reset to reboot.");

    /* halt forever */
    __asm__ volatile ("cli; hlt");
    while(1);
}
