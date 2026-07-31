#include <kernel/idt.h>
#include <driver/keybord.h>
#include <driver/vga.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kernel/heap.h>
#include <init/init.h>
#include <kernel/vfs.h>
#include <kernel/initrd.h>
#include <kernel/multiboot.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <user/tss.h>
#include <user/usermode.h>
#include <kernel/elf.h>

extern struct dentry *vfs_root;
extern uint32_t mboot_magic;
extern uint32_t mboot_addr;

void kernel_main(void)
{
        uint32_t magic = mboot_magic;
        uint32_t addr = mboot_addr;
        multiboot_t *mb;
        uint32_t mod_start;
        struct file *f;
        uint8_t *elf_data;
        uint32_t entry;
        static uint32_t kernel_stack_for_tss[1024];
        char buf[256];
        ssize_t bytes;

        serial_init();
        serial_write("Kernel started\n");

        paging_init(0x00100000, 0x01000000);
        heap_init();
        vga_init();
        idt_init();

        task_init();
        tss_init();
        tss_set_kernel_stack((uint32_t)(kernel_stack_for_tss + 1024));
        sched_init();
        pit_init();
        keybord_init();
        syscall_init();
        __asm__ volatile ("cli");

        if (magic == MULTIBOOT_BOOTLOADER_MAGIC && addr) {
                mb = (multiboot_t *)addr;
                if (mb->mods_count > 0) {
                        multiboot_module_t *mod = (multiboot_module_t *)mb->mods_addr;
                        mod_start = mod->mod_start;
                        vfs_root = initialise_initrd(mod_start);
                }
        }

        if (vfs_root) {
                f = vfs_open("shell.elf", O_RDONLY);
                if (f) {
                        elf_data = (uint8_t *)kmalloc(65536);
                        if (elf_data) {
                                bytes = vfs_read(f, (char *)elf_data, 65535);
                                if (bytes > 0) {
                                        if (elf_load(elf_data, &entry) == 0) {
                                                uint32_t user_stack_phys;
                                                uint32_t user_stack_virt;

                                                user_stack_phys = alloc_page_frame();
                                                if (!user_stack_phys) {
                                                        serial_write("Failed to allocate user stack\n");
                                                        while (1) __asm__ volatile ("hlt");
                                                }
                                                user_stack_virt = 0xBFFFF000;
                                                map_page((void *)user_stack_virt,
                                                         (void *)user_stack_phys,
                                                         PTE_PRESENT | PTE_RW | PTE_USER);

                                                switch_to_user(entry, user_stack_virt + 4096);
                                        }
                                }
                                kfree(elf_data);
                        }
                        vfs_close(f);
                }
        }
        init_start();

        while (1) {
                __asm__ volatile ("hlt");
        }
}
