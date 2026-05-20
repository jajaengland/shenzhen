#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "kprintf.h"
#include "pmm.h"
#include "exceptions.h"
#include "heap.h"
#include "process.h"
#include "timer.h"
#include "multiboot.h"

/* Scancode set 1 — unshifted */
static const char scancode_map[58] = {
    0,   27,  '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

/* Shifted equivalents */
static const char scancode_shift[58] = {
    0,   27,  '!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
};

/* Special scancodes */
#define SC_LSHIFT    0x2A
#define SC_RSHIFT    0x36
#define SC_LSHIFT_R  0xAA
#define SC_RSHIFT_R  0xB6
#define SC_CAPSLOCK  0x3A
#define SC_UP        0x48
#define SC_DOWN      0x50
#define SC_EXTENDED  0xE0

static int shift_held = 0;
static int caps_lock  = 0;
static int extended   = 0;   /* next scancode is extended (E0 prefix) */

/* Key state — returns printable char, 0 for nothing, or special codes */
#define KEY_UP   0x01   /* use non-printable values as sentinels */
#define KEY_DOWN 0x02

static int poll_key(void) {
    if (!(inb(0x64) & 0x01)) return 0;
    uint8_t sc = inb(0x60);

    /* extended key prefix */
    if (sc == SC_EXTENDED) { extended = 1; return 0; }

    if (extended) {
        extended = 0;
        if (sc == SC_UP)   return KEY_UP;
        if (sc == SC_DOWN) return KEY_DOWN;
        return 0;
    }

    /* shift press/release */
    if (sc == SC_LSHIFT || sc == SC_RSHIFT)   { shift_held = 1; return 0; }
    if (sc == SC_LSHIFT_R || sc == SC_RSHIFT_R) { shift_held = 0; return 0; }

    /* caps lock toggle on press only */
    if (sc == SC_CAPSLOCK) { caps_lock = !caps_lock; return 0; }

    /* ignore key releases */
    if (sc & 0x80) return 0;
    if (sc >= 58)  return 0;

    char c = shift_held ? scancode_shift[sc] : scancode_map[sc];

    /* caps lock only affects letters */
    if (caps_lock && c >= 'a' && c <= 'z') c -= 32;
    if (caps_lock && c >= 'A' && c <= 'Z' && !shift_held) c += 32;

    return (int)(unsigned char)c;
}

#define MAX_CMD     256
#define HISTORY_MAX 16

static char history[HISTORY_MAX][MAX_CMD];
static int  history_count = 0;
static int  history_pos   = -1;  /* -1 = not browsing */

static int kstrcmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int kstrlen(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

static void history_add(const char *cmd) {
    if (cmd[0] == '\0') return;
    /* don't add if same as most recent entry */
    if (history_count > 0 && kstrcmp(history[0], cmd) == 0) return;
    /* shift history up */
    if (history_count < HISTORY_MAX) history_count++;
    for (int i = history_count - 1; i > 0; i--) {
        const char *src = history[i-1];
        char *dst = history[i];
        int j = 0;
        while ((*dst++ = *src++)) j++;
    }
    /* copy new command into slot 0 */
    int i = 0;
    while ((history[0][i] = cmd[i])) i++;
}

static void test_process(void) {
    for (int i = 0; i < 5; i++) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        kprintf("[test process tick %d]\n", i);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        /* busy wait so timer can preempt us */
        for (volatile int j = 0; j < 5000000; j++);
    }
}

static void run_command(const char *cmd) {
    if (kstrcmp(cmd, "help") == 0) {
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_println("Commands:");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_println("  help    - show this message");
        vga_println("  clear   - clear the screen");
        vga_println("  about   - about this OS");
        vga_println("  mem     - show memory info");
        vga_println("  memtest - test heap allocator");
        vga_println("  ps      - list processes");
        vga_println("  spawn   - spawn a test process");
        vga_println("  yield   - run next process");
        vga_println("  panic   - trigger a test kernel panic");
    } else if (kstrcmp(cmd, "clear") == 0) {
        vga_clear();
    } else if (kstrcmp(cmd, "mem") == 0) {
        size_t free  = pmm_free_pages()  * PAGE_SIZE / 1024;
        size_t total = pmm_total_pages() * PAGE_SIZE / 1024;
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        kprintf("Memory: %u KB free / %u KB total\n", free, total);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    } else if (kstrcmp(cmd, "memtest") == 0) {
        void *a = kmalloc(64);
        void *b = kmalloc(128);
        kprintf("alloc 64b  at %X\n", (uint32_t)a);
        kprintf("alloc 128b at %X\n", (uint32_t)b);
        kfree(a);
        kfree(b);
        void *c = kmalloc(64);
        kprintf("realloc 64b at %X (should match first)\n", (uint32_t)c);
        kfree(c);
        vga_set_color(VGA_GREEN, VGA_BLACK);
        vga_println("heap OK");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    } else if (kstrcmp(cmd, "ps") == 0) {
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        kprintf("%-4s %-16s %-8s\n", "PID", "NAME", "STATE");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        extern process_t processes[];
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (processes[i].state == PROC_UNUSED) continue;
            const char *st = "?";
            switch (processes[i].state) {
                case PROC_RUNNING: st = "running"; break;
                case PROC_READY:   st = "ready";   break;
                case PROC_BLOCKED: st = "blocked";  break;
                case PROC_DEAD:    st = "dead";    break;
                default: break;
            }
            kprintf("%-4d %-16s %-8s\n", processes[i].pid, processes[i].name, st);
        }
    } else if (kstrcmp(cmd, "spawn") == 0) {
        process_create("test", test_process);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_println("Process created.");
    } else if (kstrcmp(cmd, "panic") == 0) {
        __asm__ volatile ("ud2");  /* invalid instruction — triggers exception 6 */
    } else if (kstrcmp(cmd, "yield") == 0) {
        yield();
    } else if (kstrcmp(cmd, "about") == 0) {
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_println("Shenzhen Operating System");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_println("A hobby x86 kernel built from scratch in C.");
        kprintf("VGA buffer at %X\n", 0xB8000);
    } else if (cmd[0] == '\0') {
        /* empty command, do nothing */
    } else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("Unknown command: ");
        vga_println(cmd);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void kstrcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

void kernel_main(struct mb_info *mb) {
    vga_init();
    vga_cursor_enable();
    gdt_init();
    idt_init();
    exceptions_init();
    pmm_init(mb);
    heap_init();
    process_init();
    timer_init(100);   /* 100 Hz = switch every 10ms */
    __asm__ volatile ("sti");

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println("Shenzhen Operating System");
    vga_println("");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_println("Kernel booted. Type 'help' for commands.");
    vga_println("");

    char cmd[MAX_CMD];
    char saved_cmd[MAX_CMD];  /* preserve what user was typing before history browse */
    int  cmd_len = 0;
    cmd[0] = '\0';
    saved_cmd[0] = '\0';

    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_print("> ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    while (1) {
        int key = poll_key();
        if (!key) continue;

        if (key == '\n') {
            cmd[cmd_len] = '\0';
            vga_putchar('\n');
            history_add(cmd);
            run_command(cmd);
            cmd_len = 0;
            cmd[0] = '\0';
            history_pos = -1;
            vga_set_color(VGA_YELLOW, VGA_BLACK);
            vga_print("> ");
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

        } else if (key == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
                cmd[cmd_len] = '\0';
                vga_backspace();
            }

        } else if (key == KEY_UP) {
            if (history_count == 0) continue;
            if (history_pos == -1) {
                kstrcpy(saved_cmd, cmd);
                history_pos = 0;
            } else if (history_pos < history_count - 1) {
                history_pos++;
            } else {
                continue;  /* already at oldest, do nothing */
            }
            /* erase what's on screen using current cmd_len */
            for (int i = 0; i < cmd_len; i++) vga_backspace();
            kstrcpy(cmd, history[history_pos]);
            cmd_len = kstrlen(cmd);
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            vga_print(cmd);

        } else if (key == KEY_DOWN) {
            if (history_pos == -1) continue;
            /* erase what's on screen */
            for (int i = 0; i < cmd_len; i++) vga_backspace();
            if (history_pos > 0) {
                history_pos--;
                kstrcpy(cmd, history[history_pos]);
            } else {
                kstrcpy(cmd, saved_cmd);
                history_pos = -1;
            }
            cmd_len = kstrlen(cmd);
            vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
            vga_print(cmd);

        } else {
            if (cmd_len < MAX_CMD - 1) {
                cmd[cmd_len++] = (char)key;
                cmd[cmd_len] = '\0';
                vga_putchar((char)key);
            }
        }
    }
}
