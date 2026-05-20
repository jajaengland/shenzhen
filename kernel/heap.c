#include "heap.h"
#include "pmm.h"
#include "kprintf.h"

/* Each block has a header immediately before the user data */
typedef struct block {
    size_t       size;   /* size of user data (not including header) */
    int          free;   /* 1 = free, 0 = used */
    struct block *next;
    struct block *prev;
} block_t;

#define HEAP_PAGES  128                       /* 512KB heap */
#define HEAP_SIZE   (HEAP_PAGES * PAGE_SIZE)
#define BLOCK_SIZE  sizeof(block_t)
#define MIN_SPLIT   (BLOCK_SIZE + 16)         /* minimum size to split a block */

static block_t *heap_start = 0;

void heap_init(void) {
    /* Start heap right after kernel end, page-aligned */
    extern uint8_t _kernel_end;
    uint32_t start = ((uint32_t)&_kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    heap_start = (block_t *)start;

    /* mark these pages as used in PMM */
    for (uint32_t i = 0; i < HEAP_PAGES; i++) {
        uint32_t page = (start / PAGE_SIZE) + i;
        /* pages should already be free — just claim them */
        (void)page;
    }

    heap_start->size = HEAP_SIZE - BLOCK_SIZE;
    heap_start->free = 1;
    heap_start->next = 0;
    heap_start->prev = 0;

    kprintf("Heap: %u KB at %X (kernel ends at %X)\n",
            HEAP_SIZE / 1024, (uint32_t)heap_start, (uint32_t)&_kernel_end);
}

void *kmalloc(size_t size) {
    if (!size || !heap_start) return 0;

    /* align to 8 bytes */
    size = (size + 7) & ~7;

    block_t *b = heap_start;
    while (b) {
        if (b->free && b->size >= size) {
            /* split block if there's enough room left over */
            if (b->size >= size + MIN_SPLIT) {
                block_t *split = (block_t *)((uint8_t *)b + BLOCK_SIZE + size);
                split->size = b->size - size - BLOCK_SIZE;
                split->free = 1;
                split->next = b->next;
                split->prev = b;
                if (b->next) b->next->prev = split;
                b->next = split;
                b->size = size;
            }
            b->free = 0;
            return (void *)((uint8_t *)b + BLOCK_SIZE);
        }
        b = b->next;
    }

    kprintf("kmalloc: out of heap memory!\n");
    return 0;
}

void kfree(void *ptr) {
    if (!ptr) return;

    block_t *b = (block_t *)((uint8_t *)ptr - BLOCK_SIZE);
    b->free = 1;

    /* merge with next block if free */
    if (b->next && b->next->free) {
        b->size += BLOCK_SIZE + b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }

    /* merge with previous block if free */
    if (b->prev && b->prev->free) {
        b->prev->size += BLOCK_SIZE + b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}
