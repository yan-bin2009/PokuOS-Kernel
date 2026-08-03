#include <driver/keybord.h>
#include <driver/vga.h>
#include <kernel/caps.h>
#include <kernel/kstring.h>
#include <kernel/paging.h>
#include <kernel/ports.h>
#include <kernel/process.h>
#include <kernel/pt_regs.h>
#include <kernel/sandbox.h>
#include <kernel/serial.h>
#include <kernel/syscall.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <stdint.h>

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

static uint32_t cap_for(int sys)
{
        switch (sys)
        {
        case SYS_WRITE:        return CAP_WRITE;
        case SYS_READ:         return CAP_READ;
        case SYS_EXIT:         return CAP_EXIT;
        case SYS_GETCHAR:      return CAP_GETCHAR;
        case SYS_PUTCHAR:      return CAP_PUTCHAR;
        case SYS_CLEAR:        return CAP_CLEAR;
        case SYS_REBOOT:       return CAP_REBOOT;
        case SYS_POWEROFF:     return CAP_POWEROFF;
        case SYS_TIER_QUERY:   return CAP_TIER_QUERY;
        case SYS_TIER_REQUEST: return CAP_TIER_REQUEST;
        case SYS_FORK:         return CAP_FORK;
        case SYS_FORK_WITH_SANDBOX: return CAP_FORK;
        case SYS_EXEC:         return CAP_EXEC;
        case SYS_WAIT:         return CAP_WAIT;
        default:               return CAP_ALL;
        }
}

static struct fork_sandbox_config kernel_sb;

static int copy_sandbox_config(uint32_t user_ptr)
{
        struct fork_sandbox_config *sb = &kernel_sb;

        if (user_ptr >= 0xC0000000)
                return -1;
        if (!user_range_valid(user_ptr, sizeof(struct fork_sandbox_config)))
                return -1;
        memcpy(sb, (void *)user_ptr, sizeof(struct fork_sandbox_config));
        return 0;
}

static int sys_exec_from_user(struct pt_regs *regs, uint32_t path_ptr)
{
        static char path[128];
        uint32_t i;

        if (path_ptr >= 0xC0000000)
                return -1;
        for (i = 0; i < sizeof(path) - 1; i++)
        {
                uint32_t page = path_ptr + i;
                uint32_t *pd = (uint32_t *)0xFFFFF000;
                uint32_t *pt;
                uint32_t pd_idx = page >> 22;
                uint32_t pt_idx = (page >> 12) & 0x3FF;

                if (!(pd[pd_idx] & PTE_PRESENT))
                        return -1;
                pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                if (!(pt[pt_idx] & PTE_PRESENT) || !(pt[pt_idx] & PTE_USER))
                        return -1;
                path[i] = *(char *)(path_ptr + i);
                if (path[i] == '\0')
                        return sys_exec(regs, path);
        }
        path[sizeof(path) - 1] = '\0';
        return -1;
}

int syscall_handler(struct pt_regs *regs)
{
        uint32_t need;

        need = cap_for(regs->eax);
        if (current_task && !(current_task->caps & need))
                return -1;

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
                task_exit((int)regs->ebx);
                return 0;
        case SYS_TIER_QUERY:
                if (current_task)
                        return (int)current_task->tier;
                return -1;
        case SYS_TIER_REQUEST:
                if (regs->ebx < TIER_KERNEL || regs->ebx > TIER_CRITICAL)
                        return -1;
                if (!current_task)
                        return -1;
                task_set_tier(current_task, (tier_t)regs->ebx);
                return (int)current_task->tier;
        case SYS_FORK:
                return sys_fork(regs, NULL);
        case SYS_FORK_WITH_SANDBOX:
                if (copy_sandbox_config((uint32_t)regs->ebx) != 0)
                        return -1;
                return sys_fork(regs, &kernel_sb);
        case SYS_EXEC:
                return sys_exec_from_user(regs, (uint32_t)regs->ebx);
        case SYS_WAIT:
                return sys_wait((int)regs->ebx);
        default:
                return -1;
        }
}

void syscall_init(void)
{
}
