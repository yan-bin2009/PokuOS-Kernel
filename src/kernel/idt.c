#include <kernel/idt.h>
#include <kernel/ports.h>
#include <kernel/serial.h>
#include <stdint.h>

struct idt_entry {
        unsigned short base_low;
        unsigned short sel;
        unsigned char always0;
        unsigned char flags;
        unsigned short base_high;
} __attribute__((packed));

struct idt_ptr
{
        unsigned short limit;
        unsigned int base;
} __attribute__((packed));

struct idt_entry idt[256] __attribute__((section(".data")));
struct idt_ptr idtp;

extern void keybord_entry(void);
extern void pit_entry(void);
extern void syscall_entry(void);

void __attribute__((interrupt)) generic_exception_handler(void *frame)
{
        serial_write("Exception!\n");
        while (1) __asm__ volatile ("hlt");
}

void __attribute__((interrupt)) user_test_int_handler(void *frame)
{
        serial_write("User mode interrupt triggered!\n");
}

void idt_set_gate(unsigned char num, unsigned long base,
                  unsigned short sel, unsigned char flags)
{
        idt[num].base_low = (base & 0xFFFF);
        idt[num].base_high = (base >> 16) & 0xFFFF;
        idt[num].sel = sel;
        idt[num].always0 = 0;
        idt[num].flags = flags;
}

void idt_init(void)
{
        uint32_t addr = (uint32_t)syscall_entry;
        int i;

        idt[0x80].base_low = addr & 0xFFFF;
        idt[0x80].base_high = (addr >> 16) & 0xFFFF;
        idt[0x80].sel = 0x08;
        idt[0x80].always0 = 0;
        idt[0x80].flags = 0xEE;

        idtp.limit = sizeof(struct idt_entry) * 256 - 1;
        idtp.base = (unsigned int)&idt;
        serial_write("IDT[0x80].base_low=");
        serial_write_hex(idt[0x80].base_low);
        serial_write(" base_high=");
        serial_write_hex(idt[0x80].base_high);
        serial_write(" flags=");
        serial_write_hex(idt[0x80].flags);
        serial_write("\n");

        for (i = 0; i < 32; i++)
                idt_set_gate(i, (unsigned long)generic_exception_handler,
                             0x08, GATE_INTERRUPT);

        idt_set_gate(IRQ1_VECTOR, (unsigned long)keybord_entry,
                     0x08, GATE_INTERRUPT);
        idt_set_gate(32, (unsigned long)pit_entry,
                     0x08, GATE_INTERRUPT);
        idt_set_gate(0x90, (unsigned long)user_test_int_handler,
                     0x08, GATE_USER);
        __asm__ volatile ("lidt (%0)" : : "r" (&idtp));
}
