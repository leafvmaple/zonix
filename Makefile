INC=boot

AS =as
CC =gcc
CFLAGS =-Wall -O2 -fomit-frame-pointer -fno-builtin

.s.o:
	$(AS) -c -o $*.o $<

all: Boot.img

clean:
	rm -f boot/*.o

Boot.img: boot/bootsec.bin

boot/bootsec.bin: boot/bootsect.s
	$(AS) -o boot/bootsect.bin boot/bootsect.s
