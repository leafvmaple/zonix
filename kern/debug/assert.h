#pragma once

#define panic(...) \
    __panic(__FILE__, __LINE__, __VA_ARGS__)

#define assert(x)                              \
    do {                                       \
        if (!(x)) {                            \
            panic("assertion failed: %s", #x); \
        }                                      \
    } while (0)