CC = gcc
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float -I. -I../include
LDFLAGS = -m elf_i386 -T user.ld -nostdlib

SRCS = $(filter-out tss.c,$(wildcard *.c))
TARGETS = $(SRCS:.c=.elf)

all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lib/crt0.o: lib/crt0.c
	$(CC) $(CFLAGS) -c $< -o $@

%.elf: %.o lib/crt0.o
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o *.elf lib/*.o

initrd: $(TARGETS)
	cp *.elf ../initrd/

.PHONY: all clean initrd
