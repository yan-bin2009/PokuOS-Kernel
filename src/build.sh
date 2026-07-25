#!/bin/bash
set -e

mkdir -p bin

# 汇编 boot.asm
nasm -f elf32 boot/boot.asm -o bin/boot.o

# 汇编 task.asm 生成 switch_to.o
nasm -f elf32 kernel/task.asm -o bin/switch_to.o

# 编译所有 kernel/*.c
for cfile in kernel/*.c; do
    objfile="bin/$(basename ${cfile%.c}.o)"
    gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float \
        -I./driver -I./kernel -c "$cfile" -o "$objfile"
done

# 编译所有 driver/*.c
for cfile in driver/*.c; do
    objfile="bin/$(basename ${cfile%.c}.o)"
    gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float \
        -I./driver -I./kernel -c "$cfile" -o "$objfile"
done

# 链接所有 .o 文件（包括 switch_to.o）
ld -m elf_i386 -T linker.ld -no-pie bin/*.o -o kernel.bin

echo "Build successful! kernel.bin is ready."
