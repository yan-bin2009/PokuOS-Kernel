#include <driver/keybord.h>
#include <driver/vga.h>
#include <init/init.h>
#include <kernel/elf.h>
#include <kernel/heap.h>
#include <kernel/idt.h>
#include <kernel/initrd.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/syscall.h>
#include <kernel/vfs.h>
#include <user/tss.h>
#include <user/usermode.h>
#include <vm/vm.h>

extern struct dentry *vfs_root;
extern uint32_t mboot_magic;
extern uint32_t mboot_addr;
extern uint32_t page_directory[];
extern task_t *current_task;

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
        // 出屎化
        serial_init();
        serial_write("Kernel started\n");

        paging_init(0x00100000, 0x01000000);
        heap_init();
        heap_selftest();
        vga_init();
        idt_init();

        task_init();
        tss_init();
        tss_set_kernel_stack((uint32_t)(kernel_stack_for_tss + 1024));
        sched_init();
        pit_init();
        keybord_init();
        syscall_init();
        vm_init();
        __asm__ volatile("cli");

        if (magic == MULTIBOOT_BOOTLOADER_MAGIC && addr)
        {
                mb = (multiboot_t *)addr;
                if (mb->mods_count > 0)
                {
                        multiboot_module_t *mod = (multiboot_module_t *)mb->mods_addr;
                        mod_start = mod->mod_start;
                        vfs_root = initialise_initrd(mod_start);
                }
        }

        if (vfs_root)
        {
                f = vfs_open("motd", O_RDONLY);
                if (f)
                {
                        bytes = vfs_read(f, buf, 255);
                        if (bytes > 0)
                        {
                                buf[bytes] = '\0';
                                serial_write(buf);
                                serial_write("\n");
                        }
                        vfs_close(f);
                }

                f = vfs_open("shell.elf", O_RDONLY);
                if (f)
                {
                        elf_data = (uint8_t *)kmalloc(65536);
                        if (elf_data)
                        {
                                bytes = vfs_read(f, (char *)elf_data, 65535);
                                if (bytes > 0)
                                {
                                        uint32_t task_pd;
                                        uint32_t user_stack_phys;
                                        uint32_t user_stack_virt;

                                        task_pd = paging_create_task_pd();
                                        if (!task_pd)
                                        {
                                                serial_write("Failed to create task page directory\n");
                                                while (1)
                                                        __asm__ volatile("hlt");
                                        }
                                        load_cr3(task_pd);
                                        current_task->cr3 = task_pd;

                                        if (elf_load(elf_data, &entry) == 0)
                                        {
                                                user_stack_phys = alloc_page_frame();
                                                if (!user_stack_phys)
                                                {
                                                        serial_write("Failed to allocate user stack\n");
                                                        while (1)
                                                                __asm__ volatile("hlt");
                                                }
                                                user_stack_virt = 0xBFFFF000;
                                                map_page((void *)user_stack_virt,
                                                         (void *)user_stack_phys,
                                                         PTE_PRESENT | PTE_RW | PTE_USER);

                                                current_task->map = vm_map_create();
                                                if (current_task->map)
                                                        vm_protect_readonly(current_task->map);

                                                switch_to_user(entry, user_stack_virt + 4096);
                                        }

                                        /* 若加载失败则停在内核，不允许回退到内核页表 */
                                        while (1)
                                                __asm__ volatile("hlt");
                                }
                                kfree(elf_data);
                        }
                        vfs_close(f);
                }
        }
        init_start();
        // 不要改这个嵌套，你不感觉我造了一个大山吗
        while (1)
        {
                __asm__ volatile("hlt");
        }
}
