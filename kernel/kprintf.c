#include "kprintf.h"
#include "vga.h"
#include <stdarg.h>

static int kstrlen(const char *s) {
    int n = 0; while (*s++) n++; return n;
}

static void print_uint(unsigned int n, int base) {
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    int  i = 0;
    if (n == 0) { vga_putchar('0'); return; }
    while (n > 0) { buf[i++] = digits[n % base]; n /= base; }
    while (i-- > 0) vga_putchar(buf[i]);
}

static int uint_len(unsigned int n, int base) {
    if (n == 0) return 1;
    int len = 0;
    while (n > 0) { len++; n /= base; }
    return len;
}

static void print_int(int n) {
    if (n < 0) { vga_putchar('-'); print_uint((unsigned int)(-n), 10); }
    else print_uint((unsigned int)n, 10);
}

static void print_str(const char *s) {
    if (!s) { vga_print("(null)"); return; }
    vga_print(s);
}

/*
 * Supported format specifiers:
 *   %d  %u  %x  %X  %b  %c  %s  %%
 *   Width: %8d, %-8d (left align), %08d (zero pad)
 */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%') { vga_putchar(*fmt++); continue; }
        fmt++;  /* skip '%' */

        /* parse flags */
        int left_align = 0, zero_pad = 0;
        if (*fmt == '-') { left_align = 1; fmt++; }
        if (*fmt == '0') { zero_pad  = 1; fmt++; }

        /* parse width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt - '0'); fmt++; }

        char pad = (zero_pad && !left_align) ? '0' : ' ';

        switch (*fmt) {
            case 'd': {
                int v = va_arg(args, int);
                int len = uint_len(v < 0 ? (unsigned int)(-v) : (unsigned int)v, 10) + (v < 0 ? 1 : 0);
                if (!left_align) for (int i = len; i < width; i++) vga_putchar(pad);
                print_int(v);
                if (left_align)  for (int i = len; i < width; i++) vga_putchar(' ');
                break;
            }
            case 'u': {
                unsigned int v = va_arg(args, unsigned int);
                int len = uint_len(v, 10);
                if (!left_align) for (int i = len; i < width; i++) vga_putchar(pad);
                print_uint(v, 10);
                if (left_align)  for (int i = len; i < width; i++) vga_putchar(' ');
                break;
            }
            case 'x': {
                unsigned int v = va_arg(args, unsigned int);
                int len = uint_len(v, 16);
                if (!left_align) for (int i = len; i < width; i++) vga_putchar(pad);
                print_uint(v, 16);
                if (left_align)  for (int i = len; i < width; i++) vga_putchar(' ');
                break;
            }
            case 'X':
                vga_print("0x");
                print_uint(va_arg(args, unsigned int), 16);
                break;
            case 'b':
                print_uint(va_arg(args, unsigned int), 2);
                break;
            case 'c':
                vga_putchar((char)va_arg(args, int));
                break;
            case 's': {
                const char *s = va_arg(args, const char *);
                if (!s) s = "(null)";
                int len = kstrlen(s);
                if (!left_align) for (int i = len; i < width; i++) vga_putchar(' ');
                print_str(s);
                if (left_align)  for (int i = len; i < width; i++) vga_putchar(' ');
                break;
            }
            case '%': vga_putchar('%'); break;
            default:  vga_putchar('%'); vga_putchar(*fmt); break;
        }
        fmt++;
    }

    va_end(args);
}
