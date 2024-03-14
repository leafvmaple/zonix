INC=boot

AS := as
CC := gcc
LD := ld

CFLAGS  := -Wall -O2 -fomit-frame-pointer -fno-builtin -ggdb -m32 -gstabs
LDFLAGS	:= -m $(shell $(LD) -V | grep elf_i386 2>/dev/null) -nostdlib

CTYPE	:= c s

SLASH	:= /
BINDIR  := bin
OBJDIR  := obj
OBJPREFIX := __objs_

ALLOBJS	:=
TARGETS :=

listf = $(filter $(if $(2),$(addprefix %.,$(2)),%),$(wildcard $(addsuffix $(SLASH)*,$(1))))
listf_cc = $(call listf,$(1),$(CTYPE))

# (name1 name2...) -> bin/$(name1) bin/$(name2)
tobin = $(addprefix $(BINDIR)$(SLASH),$(1))

# (name1.* name2.*...) -> bin/name1.o bin/name2.o
# (name1.* name2.*..., dir) -> bin/$(dir)/$(name1).o bin/$(dir)/$(name2).o)
toobj = $(addprefix $(OBJDIR)$(SLASH)$(if $(2),$(2)$(SLASH)),$(addsuffix .o,$(basename $(1))))

# (name1.* name2.*...) -> bin/name1.d bin/name2.d
# (name1.* name2.*..., dir) -> bin/$(dir)/$(name1).d bin/$(dir)/$(name2).d)
todep = $(patsubst %.o,%.d,$(call toobj,$(1),$(2)))

# () -> __objs_
# (name1 name2) -> __objs_$(name1), __objs_$(name2)
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# (file, cc[, flags, dir])
define cc_template
$$(info $$@)
$$(call todep,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@$(2) -I$$(dir $(1)) $(3) -MM $$< -MT "$$(patsubst %.d,%.o,$$@) $$@"> $$@
$$(call toobj,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@echo + cc $$<
	$(V)$(2) -I$$(dir $(1)) $(3) -c $$< -o $$@
ALLOBJS += $$(call toobj,$(1),$(4))
endef

# (#files, cc[, flags, dir])
define do_cc_compile
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(4))))
endef

define do_add_target
__target__ = $(call tobin,$(1))
TARGETS += $$(__target__)
endef

# compile file: (#files, cc[, flags])
cc_compile = $(eval $(call do_cc_compile,$(1),$(2),$(3),$(4)))

add_target = $(eval $(call do_add_target,$(1)))


bootfiles = $(call listf_cc,boot)
$(info $(bootfiles))
$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc))

bootblock = $(call tobin,bootblock)

$(bootblock): $(call toobj,$(bootfiles)) | $(call tobin,sign)
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
#	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
#	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
#	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)

$(call add_target,bootblock)

TARGETS: $(TARGETS)

define finish_all
ALLDEPS = $$(ALLOBJS:.o=.d)
$$(sort $$(dir $$(ALLOBJS)) $(BINDIR)$(SLASH) $(OBJDIR)$(SLASH)):
	@$(MKDIR) $$@
endef

$(eval $(call finish_all))

-include $(ALLDEPS)

clean:
	rm -f boot/*.o