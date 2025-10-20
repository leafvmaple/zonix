#include "stdio.h"
#include "cons.h"
#include "stdarg.h"
#include <base/types.h>
#include "math.h"

#define FMT_NONE 0
#define FMT_TRANSFER 1

// Helper to get number of digits
static int get_num_digits(uint64_t num, int base) {
    int digits = 0;
    if (num == 0) return 1;
    while (num > 0) {
        digits++;
        uint64_t temp = num;
        do_div(temp, base);
        num = temp;
    }
    return digits;
}

void print_digit(uint64_t num, int base, int width, char padc) {
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

void print_digit_no_pad(uint64_t num, int base) {
    uint32_t mod = 0;
    if (num >= base) {
        mod = do_div(num, base);
        print_digit_no_pad(num, base);
    }
    else {
        mod = num;
    }
    cons_putc(mod < 10 ? '0' + mod : 'A' + mod - 10);
}

void print_num(va_list* args, int base, int lflag, int width, char padc, int left_align) {
    uint64_t num = lflag ? va_arg(*args, uint64_t) : va_arg(*args, uint32_t);
    
    if (left_align) {
        // Left-aligned: print number first, then padding
        int num_digits = get_num_digits(num, base);
        print_digit_no_pad(num, base);
        for (int i = num_digits; i < width; i++) {
            cons_putc(' ');
        }
    } else {
        // Right-aligned: use original function
        print_digit(num, base, width, padc);
    }
}

void print_str(va_list* args, int width, int left_align) {
    char* s = va_arg(*args, char*);
    int len = 0;
    char* p = s;
    
    // Calculate string length
    while (*p++) len++;
    
    // Print left padding if right-aligned
    if (!left_align && width > len) {
        for (int i = 0; i < width - len; i++) {
            cons_putc(' ');
        }
    }
    
    // Print string
    while (*s) {
        cons_putc(*s++);
    }
    
    // Print right padding if left-aligned
    if (left_align && width > len) {
        for (int i = 0; i < width - len; i++) {
            cons_putc(' ');
        }
    }
}

int cprintf(const char *fmt, ...) {
    char c;
    int status = FMT_NONE;
    int lflag, width, left_align;
    char padc;
    va_list args;
    va_start(args, fmt);

    while ((c = *fmt++)) {
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
            left_align = 0;
            break;
        case FMT_TRANSFER:
            switch (c) {
            case '-':
                left_align = 1;
                break;
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
            case 'p':
                print_num(&args, 16, lflag, width, padc, left_align);
                status = FMT_NONE;
                break;
            case 'd':
                print_num(&args, 10, lflag, width, padc, left_align);
                status = FMT_NONE;
                break;
            case 's':
                print_str(&args, width, left_align);
                status = FMT_NONE;
                break;
            case 'c':
                cons_putc(va_arg(args, int));
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

    return 0; // TODO: return the number of characters printed
}
