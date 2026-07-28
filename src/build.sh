#!/bin/bash
set -e

mkdir -p bin

# 编译汇编
nasm -f elf32 boot/boot.asm -o bin/boot.o
nasm -f elf32 kernel/task.asm -o bin/switch_to.o
nasm -f elf32 kernel/user.asm -o bin/user.o
nasm -f elf32 kernel/user_prog.asm -o bin/user_prog.o

# 编译所有 C 文件
for cfile in kernel/*.c init/*.c driver/*.c; do
    objfile="bin/$(basename ${cfile%.c}.o)"
    gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone \
        -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float \
        -I./include -c "$cfile" -o "$objfile"
done

# 链接
ld -m elf_i386 -T linker.ld -no-pie bin/*.o -o kernel.bin

echo "Build successful! kernel.bin is ready."

# 如果参数是 iso，则生成可启动 ISO
if [ "$1" = "iso" ]; then
    echo "Building ISO..."
    echo "Hello from PokuOS!" > test.txt
    ./tools/make_initrd test.txt hello.txt

    mkdir -p iso/boot/grub
    cp kernel.bin iso/boot/
    cp initrd.img iso/boot/

    cat > iso/boot/grub/grub.cfg << "EOF"
set timeout=0
set default=0

menuentry "PokuOS" {
    multiboot /boot/kernel.bin
    module /boot/initrd.img
}
EOF

    grub-mkrescue -o pokuos.iso iso/
    echo "ISO ready: pokuos.iso"
fi
