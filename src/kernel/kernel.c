#include <kernel/idt.h>
#include <driver/keybord.h>
#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kernel/heap.h>    //新增heap.h
#include <init/init.h>
#include <kernel/vfs.h>
#include <kernel/initrd.h>
#include <kernel/multiboot.h>

extern struct dentry *vfs_root;

void kernel_main(unsigned int magic, unsigned int addr)
{
        //测试：显存
        /*char *video = (char*)0xB8000;
        video[0] = 'P';
        video[1] = 0x0F;
        video[2] = 'O';
        video[3] = 0x0F;
        video[4] = 'K';
        video[5] = 0x0F;
        video[6] = 'U';
        video[7] = 0x0F;
        video[8] = '!';
        video[9] = 0x0F;
        */
        unsigned long mem_start = 0x00100000; // 1 MB
        unsigned long mem_end   = 0x01000000; // 16 MB
        multiboot_t *mb;
	uint32_t mod_start;

        paging_init(mem_start, mem_end);
	heap_init();

        vga_init();
        
        /*分页和堆初始化*/

        
        	if (magic == MULTIBOOT_BOOTLOADER_MAGIC && addr) {
		mb = (multiboot_t *)addr;
		if (mb->mods_count > 0) {
			mod_start = *((uint32_t *)mb->mods_addr);
			vfs_root = initialise_initrd(mod_start);
		}
	}
	if (vfs_root) {
		struct file *f = vfs_open("/hello.txt", O_RDONLY);
		if (f) {
			char buf[256];
			ssize_t bytes = vfs_read(f, buf, 255);
			if (bytes > 0) {
				buf[bytes] = '\0';
				vga_write("Content: ");
				vga_write(buf);
				vga_write("\n");
			}
			vfs_close(f);
		}
	}

        // 初始化中断、键盘、系统调用
        idt_init();
        keybord_init();
        syscall_init();

        // 开启中断
        __asm__ volatile ("sti");

        // 启动 init 进程/Shell
        init_start();

        while (1) {

                __asm__ volatile ("hlt");
        }
}
