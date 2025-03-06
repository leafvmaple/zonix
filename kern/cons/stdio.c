#include "stdio.h"
#include "cons.h"
#include "stdarg.h"
#include "types.h"
#include "math.h"

#define FMT_NONE 0
#define FMT_TRANSFER 1

int print_digit(uint64_t num, int base) {
    uint32_t mod = 0;
    if (num >= base) {
        mod = do_div(num, base);
        print_digit(num, base);
    }
    else {
        mod = num;
    }

    cons_putc(mod < 10 ? '0' + mod : 'A' + mod - 10);
}

int print_num(va_list* args, int base, int lflag) {
    uint64_t num = lflag ? va_arg(*args, uint64_t) : va_arg(*args, uint32_t);
    print_digit(num, base);
}

int cprintf(const char *fmt, ...) {
    char c;
    int status = FMT_NONE, lflag = 0;
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
            lflag = 0;
            break;
        case FMT_TRANSFER:
            switch (c) {
            case 'l':
                lflag = 1;
                break;
            case 'x':
                print_num(&args, 16, lflag);
                status = FMT_NONE;
                break;
            case 'd':
                print_num(&args, 10, lflag);
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
