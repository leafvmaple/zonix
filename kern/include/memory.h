#pragma once

#include <base/types.h>

static inline void* memset(void *s, char c, size_t n) {
    char* p = (char*)s;
    while (n-- > 0) {
        *p++ = c;
    }
    return s;
}

static inline void* memcpy(void *dst, const void *src, size_t n) {
    char *d = (char*)dst;
    const char *s = (const char*)src;
    while (n-- > 0) {
        *d++ = *s++;
    }
    return dst;
}