.SECONDEXPANSION:

CC		:= gcc
CFLAGS	:= -g -fno-builtin -Wall -ggdb -O0 -m32 -nostdinc -fno-stack-protector -fno-PIC -gdwarf-2

HOSTCC		:= gcc
HOSTCFLAGS	:= -g -Wall -O2

LD      := ld
LDFLAGS := -m elf_i386 -nostdlib

DASM = ndisasm

QEMU := qemu-system-i386

OBJDUMP := objdump
OBJCOPY := objcopy
MKDIR   := mkdir -p

TERMINAL :=wt.exe wsl

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
totarget = $(addprefix $(BINDIR)$(SLASH),$(1))

# file1 file2 -> bin/file1.bin bin/file2.bin
tobin = $(addprefix $(BINDIR)$(SLASH),$(addsuffix .bin,$(1)))

# #files, cc, cflags
# obj/src/file1.o | src/file1.c | obj/src/
#	cc -Isrc cflags -c src/file1.c -o obj/src/file1.o
# ALLOBJS += obj/src/file1.o
define compile
$$(call toobj,$(1)): $(1) | $$$$(dir $$$$@)
	$(2) -I$$(dir $(1)) $(3) -c $$< -o $$@
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
	$(MKDIR) $$@
endef
make_dir = $(eval $(call do_make_dir))

#####################################################################################

INCLUDE	+=  include  \
            kern/include

KSRCDIR :=	kern          \
            kern/arch/x86 \
			kern/debug    \
            kern/cons     \
            kern/trap     \
            kern/drivers  \
            kern/sched    \
            kern/mm


CFLAGS	+= $(addprefix -I,$(INCLUDE))

# Update build date before compilation
.PHONY: update-version
update-version:
	@bash tools/update_version.sh

$(call add_packet_files_cc,$(call listf_cc,init),initial)
$(call add_packet_files_cc,$(call listf_cc,$(KSRCDIR)),kernel)

kernel = $(call totarget,kernel)

# Ensure version is updated before compiling initial and kernel objects
$(call read_packet,initial): update-version
$(call read_packet,kernel): update-version

KOBJS = $(call read_packet,initial)
KOBJS += $(call read_packet,kernel)

$(kernel): $(KOBJS) tools/kernel.ld | $$(dir $$@)
	$(LD) $(LDFLAGS) -T tools/kernel.ld $(KOBJS) -o $@
	$(OBJDUMP) -D $@ > obj/kernel.asm
#	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > obj/kernel.sym
	$(OBJCOPY) -S -O binary $@ $(call tobin,kernel)
	$(DASM) -b 32 $(call tobin,kernel) > obj/kernel.disasm

bootfiles = $(call listf_cc,boot)
$(eval $(call compiles,$(bootfiles),$(CC),$(CFLAGS) -Os))

boot = $(call totarget,bootblock)
BOBJS = $(call toobj,$(bootfiles))

$(boot): $(BOBJS) | $$(dir $$@)
#	$(LD) $(LDFLAGS) -N -e _start -Ttext 0x7c00 -o $@ $^
	$(LD) $(LDFLAGS) -T tools/boot.ld -o $@ $^
	$(OBJDUMP) -S $@ > obj/bootblock.asm
	$(OBJCOPY) -S -O binary $@ $(call tobin,bootblock)
	$(DASM) -b 16 $(call tobin,bootblock) > obj/bootblock.disasm

$(call make_dir)

bin/zonix.img: bin/bootblock bin/kernel | $$(dir $$@)
	dd if=/dev/zero of=$@ count=8064
	dd if=bin/bootblock.bin of=$@ conv=notrunc
	dd if=bin/kernel of=$@ seek=1 conv=notrunc

# Additional disk images (4MB each = 8192 sectors)
bin/disk2.img: | $$(dir $$@)
	dd if=/dev/zero of=$@ count=8192

bin/disk3.img: | $$(dir $$@)
	dd if=/dev/zero of=$@ count=8192

bin/disk4.img: | $$(dir $$@)
	dd if=/dev/zero of=$@ count=8192

DISK_IMAGES := bin/disk2.img bin/disk3.img bin/disk4.img

TARGETS: bin/bootblock bin/kernel bin/zonix.img $(DISK_IMAGES)

.DEFAULT_GOAL := TARGETS

boot: bin/bootblock

qemu: bin/zonix.img
	$(QEMU) -S -no-reboot -monitor stdio -drive file=$<,format=raw

debug-qemu: bin/zonix.img
	$(QEMU) -S -s -parallel stdio  -drive file=$<,format=raw -serial null &
	sleep 2
	$(TERMINAL) -e "gdb -q -x tools/gdbinit"

bochs: bin/zonix.img $(DISK_IMAGES)
	bochs -q -f bochsrc.bxrc

debug-bochs: bin/zonix.img $(DISK_IMAGES)
	bochs -q -f bochsrc_debug.bxrc -dbg

gdb: bin/zonix.img $(DISK_IMAGES)
	$(TERMINAL) -e bochs -q -f bochsrc_gdb.bxrc
	sleep 2
	gdb -q -x tools/gdbinit

clean:
	rm -f -r obj bin