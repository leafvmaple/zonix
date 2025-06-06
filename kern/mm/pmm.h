#pragma once

#include "types.h"
#include "io.h"
#include "list.h"

#define PG_RESERVED 0
#define PG_PROPERTY 1 

typedef uintptr_t pte_t;
typedef uintptr_t pde_t;

typedef struct Page {
    int ref;
    uint32_t flags;
    unsigned int property;
    list_entry_t page_link;
} Page;

typedef struct {
    const char *name;
    void (*init)();
    void (*init_memmap)(Page *base, size_t n);
    struct Page *(*alloc)(size_t n);
    void (*free)(Page *base, size_t n);
    size_t (*nr_free_pages)();
} pmm_manager;

#define le2page(le, member) \
    to_struct((le), Page, member)

typedef struct {
    list_entry_t free_list;  // the list header
    unsigned int nr_free;    // # of free pages in this free list
} free_area_t;

#define SET_BIT(page, bit) ((page)->flags |= (1 << (bit)))
#define CLEAR_BIT(page, bit) ((page)->flags &= ~(1 << (bit)))

#define SET_PAGE_RESERVED(page) (SET_BIT((page), PG_RESERVED))
#define CLEAR_PAGE_RESERVED(page) (CLEAR_BIT((page), PG_RESERVED))

extern void pmm_init();