#include "pit.h"
#include "pic.h"

#include "kernel/asm.h"
#include "asm/drivers/i8259.h"

#define IO_TIMER1 0x040  // 8253 Timer #1
#define TIMER_MODE (IO_TIMER1 + 3)  // timer mode port

long startup_time = 0;

#define TIMER_SEL0    0x00          // select counter 0
#define TIMER_RATEGEN 0x04          // mode 2, rate generator
#define TIMER_16BIT   0x30          // r/w counter 16 bits, LSB first

#define TIMER_FREQ 1193180
#define TIMER_DIV(x) (TIMER_FREQ / (x))

static uint8_t CMOS_READ(uint8_t addr) {
	outb(0x70, 0x80 | addr);
	return inb(0x71);
}

#define BCD_TO_BIN(val) (((val) & 0xF) + ((val) >> 4) * 10)

void pit_init(void) {
    struct tm time;

    do {
		time.tm_sec  = CMOS_READ(0);
		time.tm_min  = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon  = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));

    time.tm_sec  = BCD_TO_BIN(time.tm_sec);
	time.tm_min  = BCD_TO_BIN(time.tm_min);
	time.tm_hour = BCD_TO_BIN(time.tm_hour);
	time.tm_mday = BCD_TO_BIN(time.tm_mday);
	time.tm_mon  = BCD_TO_BIN(time.tm_mon);
	time.tm_year = BCD_TO_BIN(time.tm_year);

    outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);

    outb(IO_TIMER1, TIMER_DIV(100) % 256);
    outb(IO_TIMER1, TIMER_DIV(100) / 256);

    pic_enable(IRQ_TIMER);
}