#include <arch/x86/asm/seg.h>
#include <arch/x86/asm/cr.h>

#include <arch/x86/io.h>

#include <base/elf.h>

#define SECT_SIZE 512

static void waitdisk() {
    while ((inb(0x1F7) & 0xC0) != 0x40)
        ;
}

static void readsect(void *dst, uint32_t secno) {
    waitdisk();

    outb(0x1F2, 1);
    outb(0x1F3, secno & 0xFF);
    outb(0x1F4, (secno >> 8) & 0xFF);
    outb(0x1F5, (secno >> 16) & 0xFF);
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);
    outb(0x1F7, 0x20);

    waitdisk();

    insl(0x1F0, dst, SECT_SIZE / 4);
}

static void readseg(uintptr_t va, uint32_t count, uint32_t offset) {
    uintptr_t end_va = va + count;

    va -= offset % SECT_SIZE;

    uint32_t secno = (offset / SECT_SIZE) + 1;  // skip boot sector

    for (; va < end_va; va += SECT_SIZE, secno++) {
        readsect((void *)va, secno);
    }
}

void bootmain(void) {
    elfhdr* hdr = (elfhdr *)KERNEL_HEADER;
    readseg((uintptr_t)hdr, SECT_SIZE * 8, 0);

    if (hdr->e_magic != ELF_MAGIC) {
        goto bad;
    }

    proghdr* ph = (struct proghdr *)((uintptr_t)hdr + hdr->e_phoff);
    proghdr* eph = ph + hdr->e_phnum;
    for (; ph < eph; ph++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }
    ((void (*)(void))(hdr->e_entry & 0xFFFFFF))();

bad:
    while (1)
        ;
}
