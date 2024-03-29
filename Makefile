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
MKDIR   := mkdir -p

TERMINAL :=gnome-terminal

ALLOBJS	:=  # 用来最终mkdir

SLASH	:= /
OBJDIR  := obj
BINDIR	:= bin

OBJPREFIX	:= __objs_
CTYPE	:= c S

# dirs, #types
# listf =  $(wildcard $(addsuffix $(SLASH)*,$(1)))
listf = $(filter $(if $(2),$(addprefix %.,$(2)),%), $(wildcard $(addsuffix $(SLASH)*,$(1))))
# dirs,
listf_cc = $(call listf,$(1),$(CTYPE))

# name1 name2 -> __objs_$(name1)
# __objs_
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# name1.* name2.*... -> obj/name1.o obj/name2.o
# name1.* name2.*..., dir -> obj/$(dir)/$(name1).o obj/$(dir)/$(name2).o)
toobj = $(addprefix $(OBJDIR)$(SLASH)$(if $(2),$(2)$(SLASH)),$(addsuffix .o,$(basename $(1))))

# #files, cc, cflags
# obj/src/file1.o | src/file1.c | obj/src/
#	cc -Isrc cflags -c src/file1.c -o obj/src/file1.o
# ALLOBJS += obj/src/file1.o
define cc_template
$$(call toobj,$(1)): $(1) | $$$$(dir $$$$@)
	$(2) -Iinclude -I$$(dir $(1)) $(3) -c $$< -o $$@
ALLOBJS += $$(call toobj,$(1))
endef

# #files, cc, cflas, packet
# __objs_$(packet) :=
# __objs_$(packet) += obj/src/file1.o obj/src/file2.o...
#	cc -Isrc cflags -c src/file1.c -o obj/src/file1.o
#	cc -Isrc cflags -c src/file2.c -o obj/src/file2.o
define add_packet
__packet__ := $(call packetname,$(4))
__objs__ := $(call toobj,$(1))
$$(__packet__) :=
$$(__packet__) += $$(__objs__)
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3))))
endef

# #files, cc, cflas, packet
add_files = $(eval $(call add_packet,$(1),$(2),$(3),$(4)))
# #files, packet
add_files_cc = $(call add_files,$(1),$(CC),$(CFLAGS) $(2))

read_packet = $(foreach p,$(call packetname,$(1)),$($(p)))

# $(info $(call listf_cc,kern))
$(call add_files_cc,$(call listf_cc,kern),kernel)

# obj/
#	mkdir -p obj/
# bin/
#	mkdir -p bin/
# obj/src/
#	mkdir -p obj/src/
define do_make_dir
$$(sort $$(dir $$(ALLOBJS)) $(BINDIR)$(SLASH) $(OBJDIR)$(SLASH)):
	@$(MKDIR) $$@
endef
make_dir = $(eval $(call do_make_dir))

$(call make_dir)

obj/boot/:
	@mkdir -p $@

obj/sign/tools/:
	@mkdir -p $@

obj/boot/bootsects.o: boot/bootsect.S | $$(dir $$@)
	$(CC) -Iboot/ $(CFLAGS) -Iinclude/ -Os -c $< -o $@

obj/boot/bootsetup.o: boot/bootsetup.c | $$(dir $$@)
	$(CC) -Iboot/ $(CFLAGS) -Iinclude/ -Os -c $< -o $@

obj/sign/tools/sign.o: tools/sign.c | $$(dir $$@)
	$(HOSTCC) -Itools/ $(HOSTCFLAGS) -c $^ -o $@

bin/sign: obj/sign/tools/sign.o | $$(dir $$@)
	$(HOSTCC) $(HOSTCFLAGS) $^ -o $@

bin/bootblock: obj/boot/bootsects.o obj/boot/bootsetup.o | bin/sign $$(dir $$@)
	$(LD) $(LDFLAGS) -N -e _start -Ttext 0x7C00 $^ -o obj/bootblock.o
	$(OBJDUMP) -S obj/bootblock.o > obj/bootblock.asm
	$(OBJCOPY) -S -O binary obj/bootblock.o obj/bootblock.out
#	$(OBJDUMP) -t obj/bootblock.o | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > obj/bootblock.sym
	bin/sign obj/bootblock.out $@

bin/kernel: obj/kern/init.o tools/kernel.ld | $$(dir $$@)
	$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ obj/kern/init.o
	$(OBJDUMP) -S $@ > obj/kernel.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > obj/kernel.sym
	$(OBJCOPY) -S -O binary $@ bin/kernel.bin

bin/ucore.img: bin/bootblock bin/kernel | $$(dir $$@)
	dd if=/dev/zero of=$@ count=10000
	dd if=bin/bootblock of=$@ conv=notrunc
	dd if=bin/kernel.bin of=$@ seek=1 conv=notrunc

TARGETS: bin/bootblock bin/kernel bin/sign bin/ucore.img

.DEFAULT_GOAL := TARGETS

qemu: bin/ucore.img
	$(QEMU) -S -no-reboot -monitor stdio -hda $<

debug: bin/ucore.img
	$(QEMU) -S -s -parallel stdio -hda $< -serial null &
	sleep 2
	$(TERMINAL) -e "gdb -q -x tools/gdbinit"

$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc))

clean:
	rm -f -r obj bin