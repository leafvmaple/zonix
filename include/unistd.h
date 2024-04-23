#pragma once

#define T_SYSCALL 0x80

#define __NR_pause	29

#define _syscall0(type, name) \
    type name(void) {         \
    long __res;               \
    __asm__ volatile ("int $0x80" : "=a" (__res) : "0" (__NR_##name)); \
    if (__res >= 0)           \
	    return (type) __res;  \
    return -1;                \
}
