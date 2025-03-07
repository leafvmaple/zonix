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

static Page* alloc(size_t n) {
    if (n > _free.nr_free) {
        return 0;
    }
    list_entry_t *le = &_free.free_list;
    Page *page = 0;
    while ((le = list_next(le)) != &_free.free_list) {
        Page *p = le2page(le, page_link);
        if (p->property >= n) {
            page = p;
            break;
        }
    }
    if (page) {
        if (page->property > n) {
            Page *p = page + n;
            p->property = page->property - n;
            SET_PAGE_RESERVED(p);
            list_add(le, &(p->page_link));
        }
        list_del(le);
        _free.nr_free -= n;
        CLEAR_PAGE_RESERVED(page);
    }
}

static void free(Page *base, size_t n) {
}

static size_t nr_free_pages() {
    return _free.nr_free;
}

const pmm_manager simple_pmm_mgr = {
    .name = "simple",
    .init = init,
    .init_memmap = init_memmap,
    .alloc = alloc,
    .free = free,
    .nr_free_pages = nr_free_pages,
};