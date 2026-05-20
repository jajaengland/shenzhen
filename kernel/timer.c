#include "timer.h"
#include "idt.h"
#include "io.h"

#define PIC1_CMD  0x20
#define PIC_EOI   0x20
#define PIT_CMD   0x43
#define PIT_CH0   0x40
#define PIT_BASE  1193180

static uint32_t ticks = 0;

extern void irq0_handler(void);

void timer_tick(void) {
    ticks++;
    outb(PIC1_CMD, PIC_EOI);
}

void timer_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE / hz;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    idt_set_gate(32, (uint32_t)irq0_handler, 0x08, 0x8E);

    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~0x01);
}

uint32_t timer_ticks(void) { return ticks; }

