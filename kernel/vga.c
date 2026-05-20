#include "vga.h"
#include "io.h"

/* VGA text buffer lives at this physical address in x86 */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_BUFFER ((volatile uint16_t *)0xB8000)

static size_t vga_row;
static size_t vga_col;
static uint8_t vga_color;

static uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

static uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_init(void) {
    vga_row   = 0;
    vga_col   = 0;
    vga_color = make_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_BUFFER[y * VGA_WIDTH + x] = make_entry(' ', vga_color);
    vga_row = 0;
    vga_col = 0;
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = make_color(fg, bg);
}

static void vga_scroll(void) {
    /* shift every row up by one */
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            VGA_BUFFER[(y-1) * VGA_WIDTH + x] = VGA_BUFFER[y * VGA_WIDTH + x];

    /* blank the last row */
    for (size_t x = 0; x < VGA_WIDTH; x++)
        VGA_BUFFER[(VGA_HEIGHT-1) * VGA_WIDTH + x] = make_entry(' ', vga_color);

    vga_row = VGA_HEIGHT - 1;
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT) vga_scroll();
        vga_cursor_move();
        return;
    }
    if (c == '\r') { vga_col = 0; vga_cursor_move(); return; }
    VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = make_entry(c, vga_color);
    if (++vga_col == VGA_WIDTH) {
        vga_col = 0;
        if (++vga_row == VGA_HEIGHT) vga_scroll();
    }
    vga_cursor_move();
}

void vga_backspace(void) {
    if (vga_col > 0) {
        vga_col--;
        VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = make_entry(' ', vga_color);
        vga_cursor_move();
    }
}

void vga_cursor_enable(void) {
    /* cursor start scanline 14, end 15 — thin cursor at bottom of cell */
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
}

void vga_cursor_move(void) {
    uint16_t pos = vga_row * VGA_WIDTH + vga_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_print(const char *str) {
    while (*str)
        vga_putchar(*str++);
}

void vga_println(const char *str) {
    vga_print(str);
    vga_putchar('\n');
}
