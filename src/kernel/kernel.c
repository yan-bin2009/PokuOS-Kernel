#include <kernel/idt.h>
#include <driver/keybord.h>
#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <init/init.h>

void kernel_main() {
        vga_init();
        paging_init(0, 0);
        idt_init();
        keybord_init();
        syscall_init();

        __asm__ volatile ("sti");

        init_start();

        while (1) {
                __asm__ volatile ("hlt");
        }
}
