#include <driver/keybord.h>
#include <driver/vga.h>
#include <init/init.h>
#include <kernel/ports.h>

#define CMD_BUF_SIZE 128

static char cmd_buf[CMD_BUF_SIZE];
static int cmd_len = 0;

static int strcmp(const char *s1, const char *s2)
{
        while (*s1 && *s2 && *s1 == *s2)
        {
                s1++;
                s2++;
        }
        return *s1 - *s2;
}

static void process_char(char c)
{
        if (c == '\n')
        {
                cmd_buf[cmd_len] = '\0';
                vga_putchar('\n');

                if (strcmp(cmd_buf, "clear") == 0)
                {
                        vga_clear();
                }
                else if (strcmp(cmd_buf, "reboot") == 0)
                {
                        outb(0x64, 0xFE);
                }
                cmd_len = 0;
                return;
        }
        if (c == '\b')
        {
                if (cmd_len > 0)
                {
                        cmd_len--;
                        vga_putchar('\b');
                }
                return;
        }

        if (cmd_len < CMD_BUF_SIZE - 1)
        {
                cmd_buf[cmd_len++] = c;
                vga_putchar(c);
        }
}

void init_start(void)
{
        const char *msg = "Syscall: Hello from int 0x80!\n";
        int len = 30;

        vga_clear();
        __asm__ volatile(
            "mov $0, %%eax\n"
            "mov $1, %%ebx\n"
            "mov %0, %%ecx\n"
            "mov %1, %%edx\n"
            "int $0x80"
            : : "r"(msg), "r"(len)
            : "eax", "ebx", "ecx", "edx");

        while (1)
        {
                char c = getchar();

                if (c)
                        process_char(c);
                __asm__ volatile("hlt");
        }
}
