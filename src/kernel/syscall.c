#include <kernel/syscall.h>
#include <kernel/ports.h>
#include <driver/vga.h>
#include <kernel/serial.h>

#define SYS_WRITE 4
#define SYS_EXIT 1
#define MEM_LIMIT 0x00400000

static int is_valid_ptr(const void* ptr, int len) {
        unsigned long addr = (unsigned long)ptr;
        if (addr + len < addr) return 0;
        if (addr + len > MEM_LIMIT) return 0;
        return 1;
}

static int sys_write(void) {
        int fd, len;
        const char* buf;
        __asm__ volatile ("mov %%ebx, %0" : "=r"(fd));
        __asm__ volatile ("mov %%ecx, %0" : "=r"(buf));
        __asm__ volatile ("mov %%edx, %0" : "=r"(len));

        if (fd == 1 && is_valid_ptr(buf, len)) {
                for (int i = 0; i < len; i++) {
                        vga_putchar(buf[i]);
                }
                return len;
        }
        return -1;
}

static void sys_exit(void) {
        // 当前无进程管理，停止执行
        // 未来应回收当前任务并切换到下一个
        __asm__ volatile ("cli");
        while (1) {
                __asm__ volatile ("hlt");
        }
}

void __attribute__((interrupt)) syscall_handler(void* frame) {
        int syscall_no;
        int ret = -1;

        // 保存调用者寄存器（虽然 interrupt 属性可能已保存，但显式保存更安全）
        __asm__ volatile ("push %ebx; push %ecx; push %edx");

        __asm__ volatile ("mov %%eax, %0" : "=r"(syscall_no));

        switch (syscall_no) {
                case SYS_WRITE:
                        ret = sys_write();
                        break;
                case SYS_EXIT:
                        sys_exit();
                        break;
                default:
                        ret = -1;
                        break;
        }

        __asm__ volatile ("mov %0, %%eax" : : "r"(ret));
        __asm__ volatile ("pop %edx; pop %ecx; pop %ebx");
}

void syscall_init(void) {
        // 目前只启用中断，后续可增加其他初始化
        __asm__ volatile ("sti");
}
