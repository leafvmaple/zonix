#pragma once

#include "types.h"

#define ELF_MAGIC 0x464C457F

typedef struct elfhdr {
    uint32_t e_magic;  // must equal ELF_MAGIC
    uint8_t e_elf[12];
    uint16_t e_type;       // 1=relocatable, 2=executable, 3=shared object, 4=core image
    uint16_t e_machine;    // 3=x86, 4=68K, etc.
    uint32_t e_version;    // file version, always 1
    uint32_t e_entry;      // entry point if executable
    uint32_t e_phoff;      // file position of program header or 0
    uint32_t e_shoff;      // file position of section header or 0
    uint32_t e_flags;      // architecture-specific flags, usually 0
    uint16_t e_ehsize;     // size of this elf header
    uint16_t e_phentsize;  // size of an entry in program header
    uint16_t e_phnum;      // number of entries in program header or 0
    uint16_t e_shentsize;  // size of an entry in section header
    uint16_t e_shnum;      // number of entries in section header or 0
    uint16_t e_shstrndx;   // section number that contains section name strings
} elfhdr;

/* program section header */
typedef struct proghdr {
    uint32_t p_type;    // loadable code or data, dynamic linking info,etc.
    uint32_t p_offset;  // file offset of segment
    uint32_t p_va;      // virtual address to map segment
    uint32_t p_pa;      // physical address, not used
    uint32_t p_filesz;  // size of segment in file
    uint32_t p_memsz;   // size of segment in memory (bigger if contains bss�?
    uint32_t p_flags;   // read/write/execute bits
    uint32_t p_align;   // required alignment, invariably hardware page size
} proghdr;

/* values for Proghdr::p_type */
#define ELF_PT_LOAD 1

/* flag bits for Proghdr::p_flags */
#define ELF_PF_X 1
#define ELF_PF_W 2
#define ELF_PF_R 4