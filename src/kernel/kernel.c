#include "idt.h"
#include "keybord.h"
#include "vga.h"
void kernel_main() {
    idt_init();
    keybord_init();
    vga_init();

    __asm__ volatile ("sti");
     while (1) {
        char c = getchar();
        if (c) {
            vga_putchar(c);   // 替代直接操作显存
        }
        __asm__ volatile ("hlt");
    }
}
