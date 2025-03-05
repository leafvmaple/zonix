#pragma once

#include "types.h"

#define E820_MAX 20  // number of entries in E820MAP

struct e820map {
    int nr_map;
    struct {
        uint64_t addr;
        uint64_t size;
        uint32_t type;
    } __attribute__((packed)) map[E820_MAX];
};

void e820_parse();