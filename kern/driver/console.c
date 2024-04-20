#include "console.h"
#include "arch/x86/x86.h"
#include "driver/kbd.h"
#include "pic.h"

#define CGA_BASE        0x3D4
#define CGA_BUF         0xB8000

#define CRT_COLS        80

uint16_t *crt_buf = (uint16_t *)CGA_BUF;
static uint16_t crt_pos = 0;

static uint8_t normal_map[256] = {
    NO, 0x1B, '1', '2', '3', '4', '5', '6',  // 0x00
    '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  // 0x10
    'o', 'p', '[', ']', '\n', NO, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  // 0x20
    '\'', '`', NO, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', NO, '*',  // 0x30
    NO, ' ', NO, NO, NO, NO, NO, NO,
    NO, NO, NO, NO, NO, NO, NO, '7',  // 0x40
    '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', NO, NO, NO, NO,  // 0x50
    [0xC7] KEY_HOME, [0x9C] '\n' /*KP_Enter*/,
    [0xB5] '/' /*KP_Div*/, [0xC8] KEY_UP,
    [0xC9] KEY_PGUP, [0xCB] KEY_LF,
    [0xCD] KEY_RT, [0xCF] KEY_END,
    [0xD0] KEY_DN, [0xD1] KEY_PGDN,
    [0xD2] KEY_INS, [0xD3] KEY_DEL};

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

static void kbd_init(void) {
    pic_enable(IRQ_KBD);
}

static int kdb_getc(void) {
    if ((inb(KBSTATP) & 0x01) == 0)
        return -1;

    uint8_t data = inb(KBDATAP);
    return normal_map[data];
}

void cons_init() {
    cga_init();
    kbd_init();
    cprintf("Zonix OS is Loading...\n");
}

char cons_getc(void) {
    return kdb_getc();
}

void cons_putc(int c) {
    cga_putc(c);
}
