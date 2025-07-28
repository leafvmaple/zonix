#pragma once

#include "x86_def.h"

/* Assembler macros to create x86 segments */

#define GATE_DESC(type, dpl) (0x8000 + (dpl << 13) + (type << 8))

/* Normal segment */
#define GEN_SEG_NULL                                            \
    .word 0, 0;                                                 \
    .byte 0, 0, 0, 0

#define GEN_SEG_DESC(type,base,lim)                             \
    .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);          \
    .byte (((base) >> 16) & 0xff), (0x90 | (type)), (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)


