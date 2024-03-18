.SECONDEXPANSION:

CC		:= gcc
CFLAGS	:= -g -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc -fno-stack-protector

HOSTCC		:= gcc
HOSTCFLAGS	:= -g -Wall -O2

LD      := ld
LDFLAGS := -m elf_i386 -nostdlib

QEMU := qemu-system-i386

OBJDUMP := objdump
OBJCOPY := objcopy

TERMINAL :=gnome-terminal

bin/:
	@mkdir -p $@

obj/:
	@mkdir -p $@

obj/boot/:
	@mkdir -p $@

obj/sign/tools/:
	@mkdir -p $@

obj/boot/bootsects.o: boot/bootsect.s | $$(dir $$@)
	$(CC) -Iboot $(CFLAGS) -Ilibs/ -Os -c $< -o $@

obj/sign/tools/sign.o: tools/sign.c | $$(dir $$@)
	$(HOSTCC) -Itools $(HOSTCFLAGS) -c $^ -o $@

bin/sign: obj/sign/tools/sign.o | $$(dir $$@)
	$(HOSTCC) $(HOSTCFLAGS) $^ -o $@

bin/bootblock: obj/boot/bootsects.o | bin/sign $$(dir $$@)
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o obj/bootblock.o
	$(OBJDUMP) -S obj/bootblock.o > obj/bootblock.asm
	$(OBJCOPY) -S -O binary obj/bootblock.o obj/bootblock.out
#	$(OBJDUMP) -t obj/bootblock.out | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > obj/bootblock.sym
	bin/sign obj/bootblock.out $@

bin/ucore.img: bin/bootblock | $$(dir $$@)
	dd if=/dev/zero of=$@ count=10000
	dd if=bin/bootblock of=$@ conv=notrunc

TARGETS: bin/bootblock bin/sign bin/ucore.img

.DEFAULT_GOAL := TARGETS

qemu: bin/ucore.img
	$(QEMU) -S -no-reboot -monitor stdio -hda $<

debug: bin/ucore.img
	$(QEMU) -S -s -parallel stdio -hda $< -serial null &
	sleep 2
	$(TERMINAL) -e "gdb -q -x tools/gdbinit"

clean:
	rm -f -r obj bin