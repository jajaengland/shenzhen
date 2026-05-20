#include "gdt.h"

/* A GDT entry is 8 bytes */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr   gp;

/* Tell the CPU to load our new GDT and reload segment registers */
extern void gdt_flush(uint32_t);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_mid    = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init(void) {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0,          0,    0);       /* null segment */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);   /* kernel code  */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);   /* kernel data  */

    gdt_flush((uint32_t)&gp);
}
