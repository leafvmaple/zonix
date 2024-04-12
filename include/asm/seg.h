#pragma once

/* Control Register flags */
#define CR0_PE 0x00000001  // Protection Enable
#define CR0_MP 0x00000002  // Monitor coProcessor
#define CR0_EM 0x00000004  // Emulation
#define CR0_TS 0x00000008  // Task Switched
#define CR0_ET 0x00000010  // Extension Type
#define CR0_NE 0x00000020  // Numeric Errror
#define CR0_WP 0x00010000  // Write Protect
#define CR0_AM 0x00040000  // Alignment Mask
#define CR0_NW 0x20000000  // Not Writethrough
#define CR0_CD 0x40000000  // Cache Disable
#define CR0_PG 0x80000000  // Paging


#define STA_X 0x8  // Executable segment
#define STA_E 0x4  // Expand down (non-executable segments)
#define STA_C 0x4  // Conforming code segment (executable only)
#define STA_W 0x2  // Writeable (non-executable segments)
#define STA_R 0x2  // Readable (executable segments)
#define STA_A 0x1  // Accessed

/* System segment type bits */
#define STS_T16A 0x1  // Available 16-bit TSS
#define STS_LDT 0x2   // Local Descriptor Table
#define STS_T16B 0x3  // Busy 16-bit TSS
#define STS_CG16 0x4  // 16-bit Call Gate
#define STS_TG 0x5    // Task Gate / Coum Transmitions
#define STS_IG16 0x6  // 16-bit Interrupt Gate
#define STS_TG16 0x7  // 16-bit Trap Gate
#define STS_T32A 0x9  // Available 32-bit TSS
#define STS_T32B 0xB  // Busy 32-bit TSS
#define STS_CG32 0xC  // 32-bit Call Gate
#define STS_IG32 0xE  // 32-bit Interrupt Gate
#define STS_TG32 0xF  // 32-bit Trap Gate

/* Assembler macros to create x86 segments */

#define GATE_DESC(type, dpl) (0x8000 + (dpl << 13) + (type << 8))

/* Normal segment */
#define SEG_NULL_ASM                                             \
    .word 0, 0;                                                 \
    .byte 0, 0, 0, 0

#define SEG_ASM(type,base,lim)                                  \
    .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);          \
    .byte (((base) >> 16) & 0xff), (0x90 | (type)), (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)


/* Application segment type bits */
#define STA_X       0x8     // Executable segment
#define STA_E       0x4     // Expand down (non-executable segments)
#define STA_C       0x4     // Conforming code segment (executable only)
#define STA_W       0x2     // Writeable (non-executable segments)
#define STA_R       0x2     // Readable (executable segments)
#define STA_A       0x1     // Accessed


#define KERNEL_ENTRY      0x1000
#define KERNEL_SECTORS    10000


