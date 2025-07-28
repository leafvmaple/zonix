#include "simple_pmm.h"
#include "../debug/assert.h"

free_area_t _free;

static void init() {
    list_init(&_free);
    _free.nr_free = 0;
}

static void init_memmap(Page *base, size_t n) {
    for (Page* p = base; p != base + n; p++) {
        p->ref = p->flags = p->property = 0;
    }
    base->property = n;
    SET_PAGE_RESERVED(base);
    _free.nr_free += n;
    list_add_before(&_free.free_list, &(base->page_link));
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
    return page;
}

static void free(Page *base, size_t n) {
    Page* p = base;
    for (; p != base + n; p++)
        p->flags = 0;
    base->property = n;
    SET_PAGE_RESERVED(base);
    list_entry_t *le = &_free.free_list;
    list_entry_t *prev = le;
    while ((le = list_next(le)) != &_free.free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            CLEAR_PAGE_RESERVED(p);
            list_del(le);
        } else if (p + p->property == base) {
            p->property += base->property;
            CLEAR_PAGE_RESERVED(base);
            base = p;
            list_del(le);
        } else {
            prev = le;
        }
    }
    _free.nr_free += n;
    list_add(prev, &(base->page_link));
}

static size_t nr_free_pages() {
    return _free.nr_free;
}

static void check() {
    list_entry_t *le = &_free.free_list;
    size_t total_free = 0;
    while ((le = list_next(le)) != &_free.free_list) {
        Page *p = le2page(le, page_link);
        assert(PAGE_RESERVED(p));
        total_free += p->property;
    }
    assert(total_free == _free.nr_free);

    Page* p0 = alloc(5);
    assert(p0);
    assert(PAGE_RESERVED(p0));
}

const pmm_manager simple_pmm_mgr = {
    .name = "simple",
    .init = init,
    .init_memmap = init_memmap,
    .alloc = alloc,
    .free = free,
    .nr_free_pages = nr_free_pages,
    .check = check
};