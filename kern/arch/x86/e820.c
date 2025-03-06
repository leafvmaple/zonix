#include "e820.h"

#include "defs/x86/seg.h"

void e820_parse(uint64_t* max_pa) {
    struct e820map *e820map = (struct e820map *)(E820_MEM_BASE);
    for (int i = 0; i < e820map->nr_map; i++) {
        uint64_t begin = e820map->map[i].addr, end = begin + e820map->map[i].size;
        cprintf("  memory: %lx, [%lx, %lx), type = %d.\n",
            e820map->map[i].size, begin, end - 1, e820map->map[i].type);
        if (e820map->map[i].type == E820_RAM) {
            *max_pa = end;
        }
    }
}