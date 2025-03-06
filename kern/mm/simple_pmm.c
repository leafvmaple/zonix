#include "simple_pmm.h"

static void simple_init(struct Page *base, size_t n) {
}

static struct Page* simple_alloc(size_t n) {
    return 0;
}

static void simple_free(struct Page *base, size_t n) {
}

static size_t simple_nr_free_pages(void) {
    return 0;
}

const struct pmm_manager simple_pmm_mgr = {
    .name = "simple_pmm_mgr",
    .init = simple_init,
    .alloc = simple_alloc,
    .free = simple_free,
    .nr_free_pages = simple_nr_free_pages,
};