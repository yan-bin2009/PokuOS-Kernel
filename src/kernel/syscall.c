#include <driver/keybord.h>
#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/ports.h>
#include <kernel/serial.h>
#include <kernel/syscall.h>
#include <stdint.h>

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

        for (i = 0; i < len; i++)
        {
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

static int user_range_valid(uint32_t addr, uint32_t len)
{
        uint32_t end;
        uint32_t page;

        if (!len)
                return 1;
        if (addr >= 0xC0000000)
                return 0;
        end = addr + len;
        if (end < addr || end > 0xC0000000)
                return 0;
        for (page = addr & ~0xFFF; page < end; page += 0x1000)
        {
                uint32_t *pd = (uint32_t *)0xFFFFF000;
                uint32_t *pt;
                uint32_t pd_idx = page >> 22;
                uint32_t pt_idx = (page >> 12) & 0x3FF;

                if (!(pd[pd_idx] & PTE_PRESENT))
                        return 0;
                pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                if (!(pt[pt_idx] & PTE_PRESENT) || !(pt[pt_idx] & PTE_USER))
                        return 0;
        }
        return 1;
}

int syscall_handler(struct pt_regs *regs)
{
        switch (regs->eax)
        {
        case SYS_WRITE:
                if (!user_range_valid(regs->ecx, regs->edx))
                        return -1;
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
        case SYS_POWEROFF:
                outw(0x604, 0x2000);
                return 0;
        case SYS_EXIT:
                while (1)
                        __asm__ volatile("hlt");
        default:
                return -1;
        }
}

void syscall_init(void)
{
}
