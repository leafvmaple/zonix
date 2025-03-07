#include "simple_pmm.h"

free_area_t _free;

static void init() {
    list_init(&_free);
    _free.nr_free = 0;
}

static void init_memmap(Page *base, size_t n) {
    for (Page* p = base; p != base + n; p++) {
        p->ref = 0;
        p->flags = 0;
        p->property = 0;
    }
    base->property = n;
    SET_PAGE_RESERVED(base);
    list_add_before(&_free.free_list, &(base->page_link));
    _free.nr_free += n;
}

static Page* simple_alloc(size_t n) {
    return 0;
}

static void simple_free(Page *base, size_t n) {
}

static size_t simple_nr_free_pages() {
    return 0;
}

const pmm_manager simple_pmm_mgr = {
    .name = "simple",
    .init = init,
    .init_memmap = init_memmap,
    .alloc = simple_alloc,
    .free = simple_free,
    .nr_free_pages = simple_nr_free_pages,
};