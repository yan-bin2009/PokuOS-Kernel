#!/bin/bash
set -e

mkdir -p bin

nasm -f elf32 boot/boot.asm -o bin/boot.o
nasm -f elf32 kernel/task.asm -o bin/switch_to.o
nasm -f elf32 kernel/user.asm -o bin/user.o
nasm -f elf32 kernel/user_prog.asm -o bin/user_prog.o

for cfile in kernel/*.c init/*.c; do
        objfile="bin/$(basename ${cfile%.c}.o)"
        gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone \
                -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float \
                -I./include -c "$cfile" -o "$objfile"
done

for cfile in driver/*.c; do
        objfile="bin/$(basename ${cfile%.c}.o)"
        gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone \
                -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float \
                -I./include -c "$cfile" -o "$objfile"
done

ld -m elf_i386 -T linker.ld -no-pie bin/*.o -o kernel.bin

echo "Build successful! kernel.bin is ready."
