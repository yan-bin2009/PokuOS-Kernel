#include <init/init.h>
#include <driver/vga.h>
#include <driver/keybord.h>
#include <kernel/ports.h>

#define CMD_BUF_SIZE 128

static char cmd_buf[CMD_BUF_SIZE];
static int cmd_len = 0;

static int strcmp(const char* s1, const char* s2) {
	while (*s1 && *s2 && *s1 == *s2) {
		s1++; s2++;
	}
	return *s1 - *s2;
}

static void process_char(char c) {
	if (c == '\n') {
		cmd_buf[cmd_len] = '\0';
		vga_putchar('\n');

		if (strcmp(cmd_buf, "help") == 0) {
			vga_write("Available commands: help, clear, reboot\n");
		} else if (strcmp(cmd_buf, "clear") == 0) {
			vga_clear();
		} else if (strcmp(cmd_buf, "reboot") == 0) {
			vga_write("Rebooting...\n");
			outb(0x64, 0xFE);  // 重启
		} else if (cmd_len > 0) {
			vga_write("Unknown command: ");
			vga_write(cmd_buf);
			vga_write("\n");
	}

		cmd_len = 0;
		vga_write("> ");
		return;
    }
	if (c == '\b') {
		if (cmd_len > 0) {
			cmd_len--;
			vga_putchar('\b');
		}
		return;
	}

    if (cmd_len < CMD_BUF_SIZE - 1) {
	cmd_buf[cmd_len++] = c;
	vga_putchar(c);
    }
}

void init_start() {

	
        vga_clear();  //这里vga.c的clear的功能区别在与vga.c的clear是全局，这个是清理之前的启动日志....
                      //编不下去了
	vga_write("=== PokuOS Init System ===\n");
	vga_write("Type 'help' for commands.\n");
	vga_write("> ");
	
	const char* msg = "Syscall: Hello from int 0x80!\n";
	int len = 29;
	__asm__ volatile (
			"mov $4, %%eax\n"   // SYS_WRITE
			"mov $1, %%ebx\n"   // stdout
			"mov %0, %%ecx\n"
			"mov %1, %%edx\n"
			"int $0x80"
			: : "r"(msg), "r"(len)
			: "eax", "ebx", "ecx", "edx"
); 


	while (1) {
		char c = getchar();
		if (c) {
			process_char(c);
		}
		__asm__ volatile ("hlt");
	}
}
