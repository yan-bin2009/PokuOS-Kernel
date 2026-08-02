#include <kernel/idt.h>
#include <kernel/ports.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/trap.h>
#include <stddef.h>
#include <stdint.h>
#include <vm/vm.h>

struct idt_entry
{
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

void __attribute__((interrupt)) page_fault_handler(void *frame,
                                                   uint32_t error_code)
{
        uint32_t cr2;
        struct vm_map *map;

        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        map = current_task ? current_task->map : NULL;
        if (map && vm_fault(map, cr2, error_code) == 0)
                return;
        exception_trap(frame, error_code, 14);
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
        {
                void (*handler)(void);
                struct idt_entry *e;
                uint32_t base;

                handler = exc_handlers[i];
                base = (uint32_t)handler;
                e = &idt[i];
                e->base_low = base & 0xFFFF;
                e->base_high = (base >> 16) & 0xFFFF;
                e->sel = 0x08;
                e->always0 = 0;
                e->flags = GATE_INTERRUPT;
        }

        idt_set_gate(14, (unsigned long)page_fault_handler,
                     0x08, GATE_INTERRUPT);

        idt_set_gate(IRQ1_VECTOR, (unsigned long)keybord_entry,
                     0x08, GATE_INTERRUPT);
        idt_set_gate(32, (unsigned long)pit_entry,
                     0x08, GATE_INTERRUPT);
        idt_set_gate(0x90, (unsigned long)user_test_int_handler,
                     0x08, GATE_USER);
        __asm__ volatile("lidt (%0)" : : "r"(&idtp));
}
