#include <kernel/idt.h>
#include <driver/keybord.h>
#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kernel/heap.h>    // 别忘了包含 heap.h
#include <init/init.h>

void kernel_main(void)
{
    unsigned long mem_start = 0x00100000; // 1 MB
    unsigned long mem_end   = 0x01000000; // 16 MB

    vga_init();

    // 初始化分页（必须先于中断和堆）
    paging_init(mem_start, mem_end);

    // 初始化堆
    heap_init();

    // 初始化中断、键盘、系统调用
    idt_init();
    keybord_init();
    syscall_init();

    // 开启中断
    __asm__ volatile ("sti");

    // 启动 init 进程/Shell
    init_start();

    while (1) {
        __asm__ volatile ("hlt");
    }
}
