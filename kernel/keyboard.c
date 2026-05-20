#include "keyboard.h"
#include "idt.h"
#include "io.h"

#define KB_DATA_PORT 0x60
#define PIC1_CMD     0x20
#define PIC_EOI      0x20   /* end-of-interrupt signal */

#define BUF_SIZE 64

/* Simple ring buffer */
static char    kb_buf[BUF_SIZE];
static uint8_t kb_head = 0;
static uint8_t kb_tail = 0;

/* US QWERTY scancode set 1 -> ASCII (index = scancode) */
static const char scancode_map[128] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n', 0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*',
    0,   ' ',  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0
};

/* Called by irq1_handler in idt_asm.s */
void keyboard_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);

    /* ignore key-release events (bit 7 set) */
    if (scancode & 0x80) {
        outb(PIC1_CMD, PIC_EOI);
        return;
    }

    char c = scancode_map[scancode];
    if (c) {
        uint8_t next = (kb_head + 1) % BUF_SIZE;
        if (next != kb_tail) {   /* don't overflow */
            kb_buf[kb_head] = c;
            kb_head = next;
        }
    }

    outb(PIC1_CMD, PIC_EOI);
}

extern void irq1_handler(void);

void keyboard_init(void) {
    /* nothing — polling reads directly from port 0x60 */
}

int keyboard_haschar(void) {
    return kb_head != kb_tail;
}

char keyboard_getchar(void) {
    while (!keyboard_haschar());   /* block until key available */
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % BUF_SIZE;
    return c;
}
