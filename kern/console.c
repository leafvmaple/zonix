
#include "driver/console.h"
#include "arch/x86/x86.h"

#define CGA_BASE        0x3D4
#define CGA_BUF         0xB8000

#define CRT_COLS        80

#define LPTPORT         0x378

uint16_t *crt_buf = (uint16_t *)CGA_BUF;
static uint16_t crt_pos = 0;

void cga_init() {
    outb(CGA_BASE, 14);                                        
    crt_pos = inb(CGA_BASE + 1) << 8;
    outb(CGA_BASE, 15);
    crt_pos |= inb(CGA_BASE + 1);
}

void cga_putc(int c) {
    c |= 0x0700;

    switch (c & 0xFF) {
    case '\n':
        crt_pos += CRT_COLS;
    case '\r':
        crt_pos -= (crt_pos % CRT_COLS);
        break;
    default:
        crt_buf[crt_pos ++] = c;     // write the character
        break;
    }

    outb(CGA_BASE, 14);
    outb(CGA_BASE + 1, crt_pos >> 8);
    outb(CGA_BASE, 15);
    outb(CGA_BASE + 1, crt_pos);
}

void cons_init() {
    cga_init();
    cprintf("Zonix OS is Loading...\n");
}

void cons_putc(int c) {
    cga_putc(c);
}