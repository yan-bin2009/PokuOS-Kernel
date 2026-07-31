#include <stdint.h>
#include <kernel/serial.h>
#include <kernel/syscall.h>
#include <kernel/ports.h>
#include <driver/vga.h>
#include <driver/keybord.h>

// 与 syscall_entry.asm 的 pusha 压栈顺序一致：offset 0 是 EDI
struct pt_regs
{
        uint32_t edi;
        uint32_t esi;
        uint32_t ebp;
        uint32_t esp;
        uint32_t ebx;
        uint32_t edx;
        uint32_t ecx;
        uint32_t eax;
};

static void write_buf(const char *buf, uint32_t len)
{
        uint32_t i;

        for (i = 0; i < len; i++) {
                char c = buf[i];

                vga_putchar(c);
                if (c == '\n')
                        serial_write_char('\r');
                serial_write_char(c);
        }
}

static void put_char(char c)
{
        vga_putchar(c);
        if (c == '\n')
                serial_write_char('\r');
        serial_write_char(c);
}

int syscall_handler(struct pt_regs *regs)
{
        switch (regs->eax) {
        case SYS_WRITE:
                write_buf((const char *)regs->ecx, regs->edx);
                return regs->edx;
        case SYS_PUTCHAR:
                put_char((char)regs->ebx);
                return 0;
        case SYS_GETCHAR:
                return getchar();
        case SYS_CLEAR:
                vga_clear();
                return 0;
        case SYS_REBOOT:
                outb(0x64, 0xFE);
                return 0;
        case SYS_EXIT:
                while (1) __asm__ volatile ("hlt");
        default:
                return -1;
        }
}

void syscall_init(void)
{
}
