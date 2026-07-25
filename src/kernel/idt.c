#include "idt.h"
#include "ports.h"

struct idt_entry {
        unsigned short base_low;
        unsigned short sel;
        unsigned char always0;
        unsigned char flags;
        unsigned short base_high;

}__attribute__((packed));

struct idt_ptr {
        unsigned short limit;
        unsigned int base;

}__attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;
extern void keybord_handler();

static void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_init() {
    idtp.limit = sizeof(struct idt_entry) * 256 - 1;
    idtp.base = (unsigned int)&idt;

    // 0x20 + 1 = 33, IRQ1 键盘
    idt_set_gate(33, (unsigned long)keybord_handler, 0x08, 0x8E);

    __asm__ volatile ("lidt (%0)" : : "r" (&idtp));
}



