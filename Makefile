.SECONDEXPANSION:

LD     := ld
HOSTCC := gcc

LDFLAGS := -m elf_i386 -nostdlib

OBJDUMP := objdump
OBJCOPY := objcopy

bin/:
	@mkdir -p $@

obj/:
	@mkdir -p $@

obj/boot/:
	@mkdir -p $@

obj/sign/tools/:
	@mkdir -p $@

obj/sign/tools/sign.o: tools/sign.c | $$(dir $$@)
	@gcc -Itools -g -Wall -O2 -c $^ -o $@

obj/boot/bootsects.o: boot/bootsect.s | $$(dir $$@)
	@gcc -Iboot -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc -fno-stack-protector -Ilibs/ -Os -nostdinc -c $< -o $@

bin/sign: obj/sign/tools/sign.o | $$(dir $$@)
	@gcc -g -Wall -O2 $^ -o $@

bin/bootblock: obj/boot/bootsects.o | bin/sign $$(dir $$@)
	@$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o obj/bootblock.o
	@$(OBJDUMP) -S obj/bootblock.o > obj/bootblock.asm
	@$(OBJCOPY) -S -O binary obj/bootblock.o obj/bootblock.out
	@bin/sign obj/bootblock.out $@

TARGETS: bin/bootblock bin/sign

.DEFAULT_GOAL := TARGETS

clean:
	rm -f -r obj bin