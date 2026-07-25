#include <kernel/syscall.h>
#include <kernel/ports.h>
#include <driver/vga.h>

#define SYS_WRITE 4
#define SYS_EXIT 1

static void sys_write() {
	int fd;
	const char* buf;
	int len;
	__asm__ volatile ("mov %%ebx, %0" : "=r"(fd));
	__asm__ volatile ("mov %%ecx, %0" : "=r"(buf));
	__asm__ volatile ("mov %%edx, %0" : "=r"(len));

	if (fd == 1) {
		for (int i = 0; i < len; i++) {
			vga_putchar(buf[i]);
		}
	}
}

static void sys_exit() {
	__asm__ volatile ("cli");
	while (1) {
		__asm__ volatile ("hlt");
	}
}

void __attribute__((interrupt)) syscall_handler(void* frame) {
	int syscall_no;
	__asm__ volatile ("mov %%eax, %0" : "=r"(syscall_no));

	switch (syscall_no) {
		case SYS_WRITE:
			sys_write();
			break;
		case SYS_EXIT:
			sys_exit();
			break;
		default:
			break;
	}
}

void syscall_init() {
	__asm__ volatile ("sti");
}
