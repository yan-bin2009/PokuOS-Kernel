CC = gcc
LD = ld
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float -I. -I../include
LDFLAGS = -m elf_i386 -T user.ld -nostdlib

TARGETS = shell.elf forktest.elf hello.elf
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

shell.elf: shell.o
	$(LD) $(LDFLAGS) -o $@ $^

forktest.elf: fork_test.o
	$(LD) $(LDFLAGS) -o $@ $^

hello.elf: hello.o
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o *.elf

initrd: $(TARGETS)
	cp *.elf ../initrd/

.PHONY: all clean initrd
