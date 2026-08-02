CC = gcc
LD = ld
NASM = nasm
QEMU = qemu-system-i386

CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone \
	-mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float -I./include
NASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -no-pie

CSRCS = $(wildcard kernel/*.c init/*.c driver/*.c user/*.c vm/*.c)
COBJS = $(addprefix bin/, $(notdir $(CSRCS:.c=.o)))
KOBJS = bin/boot.o bin/switch_to.o bin/syscall_entry.o bin/pit_entry.o \
	bin/keybord_entry.o bin/user-mod.o
OBJS = $(KOBJS) $(COBJS)

all: kernel.bin initrd.img

run: iso
	$(QEMU) -cdrom pokuos.iso -no-reboot

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

bin/boot.o: boot/boot.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/switch_to.o: kernel/task.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/syscall_entry.o: kernel/syscall_entry.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/pit_entry.o: kernel/pit_entry.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/keybord_entry.o: kernel/keybord_entry.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/user-mod.o: user/usermod.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/%.o: kernel/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/%.o: init/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/%.o: driver/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/%.o: user/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@
bin/%.o: vm/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p bin

user/shell.elf:
	$(MAKE) -C user

tools/make_initrd: tools/make_initrd.c
	$(CC) $< -o $@

initrd.img: user/shell.elf tools/make_initrd motd
	./tools/make_initrd motd motd user/shell.elf shell.elf

iso: kernel.bin initrd.img
	rm -rf iso
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp initrd.img iso/boot/
	printf 'set timeout=0\nset default=0\n\nmenuentry "PokuOS" {\n\tmultiboot /boot/kernel.bin\n\tmodule /boot/initrd.img\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o pokuos.iso iso
	@echo "ISO ready: pokuos.iso"

clean:
	rm -rf bin kernel.bin initrd.img iso pokuos.iso tools/make_initrd
	$(MAKE) -C user clean

format-check:
	./tools/check-format.sh

format:
	./tools/check-format.sh --fix

.PHONY: all run iso clean format-check format
