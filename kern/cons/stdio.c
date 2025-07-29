#include "stdio.h"
#include "cons.h"
#include "stdarg.h"
#include "types.h"
#include "math.h"

#define FMT_NONE 0
#define FMT_TRANSFER 1

int print_digit(uint64_t num, int base, int width, char padc) {
    uint32_t mod = 0;
    if (num >= base) {
        mod = do_div(num, base);
        print_digit(num, base, width - 1, padc);
    }
    else {
        mod = num;    
        while(--width > 0) {
            cons_putc(padc);
        }
    }

    cons_putc(mod < 10 ? '0' + mod : 'A' + mod - 10);
}

int print_num(va_list* args, int base, int lflag, int width, char padc) {
    uint64_t num = lflag ? va_arg(*args, uint64_t) : va_arg(*args, uint32_t);
    print_digit(num, base, width, padc);
}

int print_str(va_list* args) {
    char* s = va_arg(*args, char*);
    while (*s) {
        cons_putc(*s++);
    }
}

int cprintf(const char *fmt, ...) {
    char c;
    int status = FMT_NONE;
    int lflag, width;
    char padc;
    va_list args;
    va_start(args, fmt);

    while (c = *fmt++) {
        switch (status)
        {
        case FMT_NONE:
            if (c == '%') {
                status = FMT_TRANSFER;
            }
            else
                cons_putc(c);
            padc = ' ';
            lflag = 0;
            width = 0;
            break;
        case FMT_TRANSFER:
            switch (c) {
            case '0':
                padc = '0';
                break;
            case '1' ... '9':
                width = width * 10 + (c - '0');
                break;
            case 'l':
                lflag = 1;
                break;
            case 'x':
                print_num(&args, 16, lflag, width, padc);
                status = FMT_NONE;
                break;
            case 'd':
                print_num(&args, 10, lflag, width, padc);
                status = FMT_NONE;
                break;
            case 's':
                print_str(&args);
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
