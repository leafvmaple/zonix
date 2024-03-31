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
# $filter(%.type1 %.type2, dir1/* dir2/*)
listf = $(filter $(if $(2),$(addprefix %.,$(2)),%), $(wildcard $(addsuffix $(SLASH)*,$(1))))
# dirs
# $filter(%.c %.S, dir1/* dir2/*)
listf_cc = $(call listf,$(1),$(CTYPE))

# name1 name2 -> __objs_$(name1)
# __objs_
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# name1.* name2.*... -> obj/name1.o obj/name2.o
# name1.* name2.*..., dir -> obj/$(dir)/$(name1).o obj/$(dir)/$(name2).o)
toobj = $(addprefix $(OBJDIR)$(SLASH)$(if $(2),$(2)$(SLASH)),$(addsuffix .o,$(basename $(1))))

# file1 file2 -> bin/file1 bin/file2
tobin = $(addprefix $(BINDIR)$(SLASH),$(1))

# #files, cc, cflags
# obj/src/file1.o | src/file1.c | obj/src/
#	cc -Isrc cflags -c src/file1.c -o obj/src/file1.o
# ALLOBJS += obj/src/file1.o
define compile
$$(call toobj,$(1)): $(1) | $$$$(dir $$$$@)
	$(2) -Iinclude -I$$(dir $(1)) $(3) -c $$< -o $$@
ALLOBJS += $$(call toobj,$(1))
endef

compiles = $$(foreach f,$(1),$$(eval $$(call compile,$$(f),$(2),$(3))))

# #files, cc, cflags, packet
# __objs_$(packet) := obj/src/file1.o obj/src/file2.o...
# obj/src/file1.o:
#	cc -Isrc -Iinclude cflags -c src/file1.c -o obj/src/file1.o
# obj/src/file2.o:
#	cc -Isrc -Iinclude cflags -c src/file2.c -o obj/src/file2.o
define add_packet
__packet__ := $(call packetname,$(4))
__objs__ := $(call toobj,$(1))
$$(__packet__) := $$(__objs__)
$(call compiles,$(1),$(2),$(3))
endef

# #packets
# obj/src/file1.o obj/src/file2.o...
read_packet = $(foreach p,$(call packetname,$(1)),$($(p)))

# #files, cc, cflas, packet
add_packet_files = $(eval $(call add_packet,$(1),$(2),$(3),$(4)))
# #files, packet
add_packet_files_cc = $(call add_packet_files,$(1),$(CC),$(CFLAGS),$(2))

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

#####################################################################################

$(call add_packet_files_cc,$(call listf_cc,kern),kernel)
$(call add_packet_files_cc,$(call listf_cc,init),initial)

kernel = $(call tobin,kernel)
KOBJS = $(call read_packet,initial)
KOBJS += $(call read_packet,kernel)

$(kernel): $(KOBJS) tools/kernel.ld
	$(LD) $(LDFLAGS) -T tools/kernel.ld $(KOBJS) -o $@
	$(OBJDUMP) -S $@ > obj/kernel.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > obj/kernel.sym
	$(OBJCOPY) -S -O binary $@ bin/kernel.bin

bootfiles = $(call listf_cc,boot)
$(eval $(call compiles,$(bootfiles),$(CC),$(CFLAGS) -Os -nostdinc))

boot = $(call tobin,bootblock)
BOBJS = $(call toobj,$(bootfiles))

$(boot): $(BOBJS) | $(call tobin,sign)
	$(LD) $(LDFLAGS) -N -e _start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
	$(OBJDUMP) -S obj/bootblock.o > obj/bootblock.asm
	$(OBJCOPY) -S -O binary obj/bootblock.o obj/bootblock.out
	bin/sign obj/bootblock.out $@

$(call make_dir)

obj/sign/tools/:
	@mkdir -p $@

obj/sign/tools/sign.o: tools/sign.c | $$(dir $$@)
	$(HOSTCC) -Itools/ $(HOSTCFLAGS) -c $^ -o $@

bin/sign: obj/sign/tools/sign.o | $$(dir $$@)
	$(HOSTCC) $(HOSTCFLAGS) $^ -o $@

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

clean:
	rm -f -r obj bin