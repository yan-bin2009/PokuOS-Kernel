CC = gcc
LD = ld
NASM = nasm
QEMU = qemu-system-i386

CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone \
	-mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float -I./include
NASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -no-pie

CSRCS = $(wildcard kernel/*.c init/*.c driver/*.c vm/*.c fs/*.c)
COBJS = $(addprefix bin/, $(notdir $(CSRCS:.c=.o)))
KOBJS = bin/boot.o bin/switch_to.o bin/syscall_entry.o bin/pit_entry.o \
	bin/keybord_entry.o bin/user-mod.o bin/tss.o
OBJS = $(KOBJS) $(COBJS)

all: kernel.bin initrd.img

disk.img:
	printf 'POKUOS-TEST-DISK' | dd of=disk.img bs=512 count=1 conv=notrunc
	dd if=/dev/zero of=disk.img bs=512 count=8192 seek=1 conv=notrunc

run: iso disk.img
	$(QEMU) -cdrom pokuos.iso -drive file=disk.img,format=raw,if=ide -no-reboot

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

bin/%.o: vm/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/tss.o: user/tss.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/%.o: fs/%.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p bin

user/shell.elf:
	$(MAKE) -C user

initrd.img: motd
	tar --format=ustar -cf initrd.img motd

big.txt:
	head -c 32768 /dev/zero | tr '\0' 'A' > big.txt

disk.img: user/shell.elf big.txt
	dd if=/dev/zero of=disk.img bs=512 count=8192
	tar --format=ustar -cf - -C user shell.elf forktest.elf hello.elf -C .. big.txt | dd of=disk.img conv=notrunc

iso: kernel.bin initrd.img
	rm -rf iso
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp initrd.img iso/boot/
	printf 'set timeout=0\nset default=0\n\nmenuentry "PokuOS" {\n\tmultiboot /boot/kernel.bin\n\tmodule /boot/initrd.img\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o pokuos.iso iso
	@echo "ISO ready: pokuos.iso"

clean:
	rm -rf bin kernel.bin initrd.img iso pokuos.iso disk.img tools/make_initrd
	$(MAKE) -C user clean

format-check:
	./tools/check-format.sh

format:
	./tools/check-format.sh --fix

.PHONY: all run iso clean format-check format
