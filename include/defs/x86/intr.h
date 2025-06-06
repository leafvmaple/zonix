#pragma once

// BIOS Interrupt

#define SMAP 0x534D4150 // 'SMAP'

#define INT_DISK 0x13   // BIOS Disk Services
#define INT_ESI  0x15   // BIOS Extended Services Interface

#define INT_DISK_DL_HDD 0x80
#define INT_DISK_AH_LAB_READ 0x42

#define INT_ESI_AX_E820 0xE820

#define INT_ESI_DESC_SIZE 20
#define INT_ESI_ERROR_CODE 0xFFFF

#define GEN_DAP(sectors,offset,segment,lba) \
    .byte 0x10, 0x00;    \
    .word sectors, offset, segment; \
    .quad lba

/* Eflags register */
#define FL_CF 0x00000001         // Carry Flag
#define FL_PF 0x00000004         // Parity Flag
#define FL_AF 0x00000010         // Auxiliary carry Flag
#define FL_ZF 0x00000040         // Zero Flag
#define FL_SF 0x00000080         // Sign Flag
#define FL_TF 0x00000100         // Trap Flag
#define FL_IF 0x00000200         // Interrupt Flag
#define FL_DF 0x00000400         // Direction Flag
#define FL_OF 0x00000800         // Overflow Flag
#define FL_IOPL_MASK 0x00003000  // I/O Privilege Level bitmask
#define FL_IOPL_0 0x00000000     //   IOPL == 0
#define FL_IOPL_1 0x00001000     //   IOPL == 1
#define FL_IOPL_2 0x00002000     //   IOPL == 2
#define FL_IOPL_3 0x00003000     //   IOPL == 3
#define FL_NT 0x00004000         // Nested Task
#define FL_RF 0x00010000         // Resume Flag
#define FL_VM 0x00020000         // Virtual 8086 mode
#define FL_AC 0x00040000         // Alignment Check
#define FL_VIF 0x00080000        // Virtual Interrupt Flag
#define FL_VIP 0x00100000        // Virtual Interrupt Pending
#define FL_ID 0x00200000         // ID flag