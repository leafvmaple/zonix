#include "e820.h"

#include "defs/x86/seg.h"

void e820_parse() {
    struct e820map *e820map = (struct e820map *)(E820_MEM_BASE + KERNEL_ENTRY);
    for (int i = 0; i < e820map->nr_map; i++) {
        uint64_t begin = e820map->map[i].addr, end = begin + e820map->map[i].size;
        cprintf("  memory: %x, [%x, %x], type = %d.\n",
            e820map->map[i].size, begin, end - 1, e820map->map[i].type);
    }
}