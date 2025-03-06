#pragma once

#include "types.h"
#include "io.h"

#define PG_SIZE 4096

#define PG_RESERVED 0
#define PG_PROPERTY 1 

struct Page {
    int ref;                     // page frame's reference counter
    uint32_t flags;              // array of flags that describe the status of the page frame
    unsigned int property;       // the num of free block, used in first fit pm manager
    // list_entry_t page_link;      // free list link
    // list_entry_t pra_page_link;  // used for pra (page replace algorithm)
    uintptr_t pra_vaddr;         // used for pra (page replace algorithm)
};

#define SET_PAGE_RESERVED(page) set_bit(PG_RESERVED, &((page)->flags))

extern void pmm_init(long start, long end);