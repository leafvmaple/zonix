#pragma once

#include <base/types.h>

void* memset(void *s, char c, size_t n) {
    char* p = (char*)s;
    while (n-- > 0) {
        *p++ = c;
    }
    return s;
}