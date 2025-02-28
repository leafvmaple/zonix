#include "stdio.h"
#include "drivers/console.h"
#include "stdarg.h"

#define FMT_NONE 0
#define FMT_TRANSFER 1

int print_num(va_list* args, int base) {
    int num = va_arg(*args, unsigned int);
    do {
        int mod = num % base;
        cons_putc(mod < 10 ? '0' + mod : 'A' + mod - 10);
        num /= base;
    } while (num);
}

int cprintf(const char *fmt, ...) {
    char c;
    int status = FMT_NONE;
    va_list args;
    va_start(args, fmt);

    while (c = *fmt++) {
        switch (status)
        {
        case FMT_NONE:
            if (c == '%')
                status = FMT_TRANSFER;
            else
                cons_putc(c);
            break;
        case FMT_TRANSFER:
            switch (c) {
            case 'x':
                print_num(&args, 16);
                status = FMT_NONE;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
    va_end(args);
}
