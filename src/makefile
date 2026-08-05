CC = gcc
LD = ld
NASM = nasm
QEMU = qemu-system-i386
CARGO = $(HOME)/.cargo/bin/cargo +nightly
RUST_LIB = rust/libkstring.a

CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone \
	-mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float -I./include
NASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -no-pie

vpath %.c kernel init driver vm fs

CSRCS = $(wildcard kernel/*.c init/*.c driver/*.c vm/*.c fs/*.c)
COBJS = $(addprefix bin/, $(notdir $(CSRCS:.c=.o)))
ASMOBJS = bin/boot.o bin/switch_to.o bin/syscall_entry.o bin/pit_entry.o \
	bin/keyboard_entry.o bin/user-mod.o bin/tss.o
OBJS = $(COBJS) $(ASMOBJS)

all: kernel.bin initrd.img disk.img

run: iso disk.img
	-pkill -9 -f qemu-system-i386 2>/dev/null; sleep 0.3
	$(QEMU) -cdrom pokuos.iso -drive file=disk.img,format=raw,if=ide -no-reboot

kernel.bin: $(OBJS) $(RUST_LIB)
	$(LD) $(LDFLAGS) $^ -o $@

rust/libkstring.a: rust/Cargo.toml rust/Cargo.lock rust/src/lib.rs rust/i686-unknown-none.json
	$(CARGO) build --quiet --release --target rust/i686-unknown-none.json \
		-Z build-std=core -Zjson-target-spec --manifest-path rust/Cargo.toml
	cp rust/target/i686-unknown-none/release/libkstring.a rust/libkstring.a

# cargo 编译较慢且独占地使用 rust/target，禁止与其它目标并行，
# 避免 -j 下链接等到一半的产物或 CPU 争抢导致 rust 构建超时/失败。
.NOTPARALLEL: rust/libkstring.a

bin/%.o: %.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin/boot.o: boot/boot.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/switch_to.o: kernel/task.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/syscall_entry.o: kernel/syscall_entry.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/pit_entry.o: kernel/pit_entry.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/keyboard_entry.o: kernel/keyboard_entry.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/user-mod.o: user/usermod.asm | bin
	$(NASM) $(NASMFLAGS) $< -o $@

bin/tss.o: user/tss.c | bin
	$(CC) $(CFLAGS) -c $< -o $@

bin:
	mkdir -p bin

user-build:
	$(MAKE) -C user

initrd.img: motd
	tar --format=ustar -cf initrd.img motd

big.txt:
	head -c 32768 /dev/zero | tr '\0' 'A' > big.txt

# order-only：disk.img 只依赖 big.txt 判断是否需要重建；
# | user-build 保证打包前 user/*.elf 已就绪，但不再强制每次重建。
disk.img: big.txt | user-build
	dd if=/dev/zero of=disk.img bs=512 count=8192
	tar --format=ustar -cf - -C user `cd user && echo *.elf uname` -C .. big.txt | dd of=disk.img conv=notrunc

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
	rm -rf rust/target rust/libkstring.a
	$(MAKE) -C user clean

format-check:
	./tools/check-format.sh

format:
	./tools/check-format.sh --fix

.PHONY: all run iso clean format-check format FORCE user-build
FORCE:
