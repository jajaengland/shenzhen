#pragma once
#include <stdint.h>

/* Multiboot info flags */
#define MB_FLAG_MEM     (1 << 0)
#define MB_FLAG_MMAP    (1 << 6)

/* Memory map entry types */
#define MB_MMAP_AVAILABLE 1

struct mb_mmap_entry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

struct mb_info {
    uint32_t flags;
    uint32_t mem_lower;   /* KB below 1MB */
    uint32_t mem_upper;   /* KB above 1MB */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    /* more fields exist but we don't need them */
} __attribute__((packed));
