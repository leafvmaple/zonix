#pragma once

// BIOS Interrupt

#define INT_DISK 0x13

#define INT_DISK_DL_HDD 0x80
#define INT_DISK_AH_LAB_READ 0x42


#define GEN_DAP(sectors,offset,segment,lba) \
    .byte 0x10, 0x00;    \
    .word sectors, offset, segment; \
    .quad lba
