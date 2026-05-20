#include "pmm.h"
#include "kprintf.h"

/* We use a bitmap: 1 bit per 4KB page.
   1 = used, 0 = free.
   Stored in a static array covering up to 4GB (128KB of bitmap). */

#define MAX_PAGES    (1024 * 1024)   /* 4GB / 4KB */
#define BITMAP_SIZE  (MAX_PAGES / 8) /* bytes needed */

static uint8_t bitmap[BITMAP_SIZE];
static size_t  total_pages = 0;
static size_t  free_pages  = 0;

/* The kernel ends here — set by linker script */
extern uint8_t _kernel_end;
#define KERNEL_END ((uint32_t)&_kernel_end)

static void bitmap_set(uint32_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(uint32_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static int bitmap_test(uint32_t page) {
    return bitmap[page / 8] & (1 << (page % 8));
}

void pmm_init(struct mb_info *mb) {
    /* mark everything as used to start */
    for (int i = 0; i < BITMAP_SIZE; i++) bitmap[i] = 0xFF;

    if (!(mb->flags & MB_FLAG_MMAP)) {
        kprintf("PMM: no memory map from GRUB!\n");
        return;
    }

    /* walk GRUB's memory map and free available regions */
    struct mb_mmap_entry *entry = (struct mb_mmap_entry *)mb->mmap_addr;
    struct mb_mmap_entry *end   = (struct mb_mmap_entry *)(mb->mmap_addr + mb->mmap_length);

    while (entry < end) {
        if (entry->type == MB_MMAP_AVAILABLE) {
            uint64_t base = entry->addr;
            uint64_t len  = entry->len;

            /* skip first 1MB (BIOS, VGA buffer, etc) */
            if (base < 0x100000) {
                if (base + len <= 0x100000) { entry = (struct mb_mmap_entry *)((uint8_t *)entry + entry->size + 4); continue; }
                len  -= (0x100000 - base);
                base  = 0x100000;
            }

            uint32_t page_start = (uint32_t)(base / PAGE_SIZE);
            uint32_t page_count = (uint32_t)(len  / PAGE_SIZE);

            for (uint32_t i = 0; i < page_count && (page_start + i) < MAX_PAGES; i++) {
                bitmap_clear(page_start + i);
                free_pages++;
                total_pages++;
            }
        }
        entry = (struct mb_mmap_entry *)((uint8_t *)entry + entry->size + 4);
    }

    /* re-mark kernel pages as used */
    uint32_t kernel_pages = (KERNEL_END + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < kernel_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
        }
    }

    /* mark bitmap itself as used */
    uint32_t bm_start = (uint32_t)bitmap / PAGE_SIZE;
    uint32_t bm_pages = (BITMAP_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = 0; i < bm_pages; i++) {
        if (!bitmap_test(bm_start + i)) {
            bitmap_set(bm_start + i);
            free_pages--;
        }
    }

    kprintf("PMM: %u MB total, %u MB free\n",
        (total_pages * PAGE_SIZE) / (1024 * 1024),
        (free_pages  * PAGE_SIZE) / (1024 * 1024));
}

void *pmm_alloc(void) {
    for (uint32_t i = 0; i < MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return 0;   /* out of memory */
}

void pmm_free(void *page) {
    uint32_t i = (uint32_t)page / PAGE_SIZE;
    if (bitmap_test(i)) {
        bitmap_clear(i);
        free_pages++;
    }
}

size_t pmm_free_pages(void)  { return free_pages; }
size_t pmm_total_pages(void) { return total_pages; }
