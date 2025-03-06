#pragma once

#include "types.h"

typedef void (*e820_cb)(uint64_t addr, uint64_t size, uint32_t type, void *arg);

void e820_traverse(e820_cb cb, void *arg);
