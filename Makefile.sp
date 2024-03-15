AS86	=as86 -0 -a
LD86	=ld86 -0

.SECONDEXPANSION:

bin/:
	mkdir -p $@

obj/:
	mkdir -p $@

obj/boot/:
	mkdir -p $@

obj/boot/bootsects.o: boot/bootsect.s | $(dir $@)
	gcc -fno-builtin -Wall -ggdb -m32 -gstabs -nostdinc -o obj/boot/bootsects.o boot/bootsect.s

bin/sign: | $(dir $@)

bin/bootblock: obj/boot/bootsects.o | bin/sign $(dir $@)

TARGETS: bin/bootblock bin/sign

.DEFAULT_GOAL := TARGETS

clean:
	-rm -r obj bin