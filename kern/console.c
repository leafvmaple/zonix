
#include "console.h"
#include "x86.h"

uint16_t *crt_buf = 0;
static uint16_t crt_pos = 0;

#define CGA_BASE        0x3D4
#define CGA_BUF         0xB8000

#define LPTPORT         0x378

void cga_init() {
    crt_buf = (uint16_t *)CGA_BUF;

    outb(CGA_BASE, 14);                                        
    crt_pos = inb(CGA_BASE + 1) << 8;
    outb(CGA_BASE, 15);
    crt_pos |= inb(CGA_BASE + 1);

    crt_buf[crt_pos] = (uint16_t) 0xA55A;  
}

void cga_putc(int c) {
    if (!(c & ~0xFF)) {
        c |= 0x0700;
    }

    crt_buf[crt_pos ++] = c;

    outb(CGA_BASE, 14);
    outb(CGA_BASE + 1, crt_pos >> 8);
    outb(CGA_BASE, 15);
    outb(CGA_BASE + 1, crt_pos);
}

void cons_init() {
    cga_init();
}

void cons_putc(int c) {
    cga_putc(c);
}