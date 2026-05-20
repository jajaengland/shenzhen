#include "idt.h"
#include "io.h"

/* PIC ports */
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed, aligned(1)));

static struct idt_entry idt[256];
static struct idt_ptr   ip;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

/* Remap the PIC so IRQs 0-15 map to IDT entries 32-47
   (by default they overlap CPU exceptions 0-15, which is bad) */
static void pic_remap(void) {
    /* start init sequence */
    outb(PIC1_CMD,  0x11); io_wait();
    outb(PIC2_CMD,  0x11); io_wait();

    /* vector offsets */
    outb(PIC1_DATA, 0x20); io_wait();   /* IRQ0-7  -> INT 32-39 */
    outb(PIC2_DATA, 0x28); io_wait();   /* IRQ8-15 -> INT 40-47 */

    /* tell PICs about each other */
    outb(PIC1_DATA, 0x04); io_wait();
    outb(PIC2_DATA, 0x02); io_wait();

    /* 8086 mode */
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    /* mask all IRQs — drivers unmask their own */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void idt_init(void) {
    ip.limit = (sizeof(struct idt_entry) * 256) - 1;
    ip.base  = (uint32_t)&idt[0];

    /* zero the whole table */
    uint8_t *p = (uint8_t *)idt;
    for (int i = 0; i < (int)sizeof(idt); i++) p[i] = 0;

    pic_remap();

    /* load IDT directly — no function call, no ABI issues */
    __asm__ volatile ("lidt %0" : : "m"(ip));
}
