#include "e820.h"

#include "defs/x86/seg.h"

#define E820_MAX 20  // number of entries in E820MAP

struct e820map {
    int nr_map;
    struct {
        uint64_t addr;
        uint64_t size;
        uint32_t type;
    } __attribute__((packed)) map[E820_MAX];
};

void e820_traverse(e820_cb cb, void *arg) {
    struct e820map *e820map = (struct e820map *)(E820_MEM_BASE + KERNEL_BASE);
    for (int i = 0; i < e820map->nr_map; i++) {
        cb(e820map->map[i].addr, e820map->map[i].size, e820map->map[i].type, arg);
    }
}