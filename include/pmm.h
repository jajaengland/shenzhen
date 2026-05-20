#pragma once
#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"

#define PAGE_SIZE 4096

void    pmm_init(struct mb_info *mb);
void   *pmm_alloc(void);        /* allocate one 4KB page, returns physical address */
void    pmm_free(void *page);   /* free a page */
size_t  pmm_free_pages(void);   /* number of free pages */
size_t  pmm_total_pages(void);  /* total pages */
