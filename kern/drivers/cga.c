#include "cga.h"

#include "types.h"
#include "io.h"

#define CGA_IDX_REG  0x3D4
#define CGA_DATA_REG 0x3D5

#define CRTC_CURSOR_HIGH 0x0E
#define CRTC_CURSOR_LOW  0x0F

#define CGA_BUF         0xB8000

#define CRT_ROWS        25
#define CRT_COLS        80

uint16_t *crt_buf = (uint16_t *)CGA_BUF;
static uint16_t crt_pos = 0;

void cga_init() {
    outb(CGA_IDX_REG, CRTC_CURSOR_HIGH);
    crt_pos = inb(CGA_DATA_REG) << 8;
    outb(CGA_IDX_REG, CRTC_CURSOR_LOW);
    crt_pos |= inb(CGA_DATA_REG);
}

void cga_putc(int c) {
    c |= 0x0700;

    switch (c & 0xFF) {
    case '\b':
        if (crt_pos > 0) {
            crt_pos--;
            crt_buf[crt_pos] = (c & ~0xff) | ' ';
        }
        break;
    case '\n':
        crt_pos += CRT_COLS;
    case '\r':
        crt_pos -= (crt_pos % CRT_COLS);
        break;
    default:
        crt_buf[crt_pos++] = c;     // write the character
        break;
    }

    outb(CGA_IDX_REG, CRTC_CURSOR_HIGH);
    outb(CGA_DATA_REG, crt_pos >> 8);
    outb(CGA_IDX_REG, CRTC_CURSOR_LOW);
    outb(CGA_DATA_REG, crt_pos);
}
