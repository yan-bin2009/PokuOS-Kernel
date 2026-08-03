#include <driver/ata.h>
#include <driver/keybord.h>
#include <driver/vga.h>
#include <fs/bdev.h>
#include <fs/mount.h>
#include <fs/ramfs.h>
#include <fs/tarfs.h>
#include <fs/vfs.h>
#include <init/init.h>
#include <kernel/elf.h>
#include <kernel/heap.h>
#include <kernel/idt.h>
#include <kernel/kstring.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/process.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/syscall.h>
#include <kernel/watchdog.h>
#include <user/tss.h>
#include <user/usermode.h>
#include <vm/vm.h>

extern struct dentry *vfs_root;
extern uint32_t mboot_magic;
extern uint32_t mboot_addr;
extern uint32_t page_directory[];
extern task_t *current_task;

static void bdev_selftest(void)
{
        struct block_device *dev;
        uint8_t buf[512];

        ata_init();

        dev = bdev_lookup("ata0");
        if (!dev)
        {
                serial_write("[bdev] ata0 not found, skip selftest\n");
                return;
        }

        memset(buf, 0, 512);
        if (bdev_read(dev, 0, buf, 1, TIER_KERNEL) != 0)
        {
                serial_write("[bdev] read LBA0 failed\n");
                return;
        }
        if (memcmp(buf + 257, "ustar", 5) == 0)
                serial_write("[bdev] LBA0 tar magic ok\n");
        else
        {
                serial_write("[bdev] LBA0 not a tar header\n");
                return;
        }

        memcpy(buf, "WRITE-BACK-TEST", 16);
        if (bdev_write(dev, 8190, buf, 1, TIER_KERNEL) != 0)
        {
                serial_write("[bdev] write LBA8190 failed\n");
                return;
        }
        memset(buf, 0, 512);
        if (bdev_read(dev, 8190, buf, 1, TIER_KERNEL) != 0)
        {
                serial_write("[bdev] read back LBA8190 failed\n");
                return;
        }
        if (memcmp(buf, "WRITE-BACK-TEST", 16) == 0)
                serial_write("[bdev] LBA8190 write/read-back ok\n");
        else
                serial_write("[bdev] LBA8190 read-back mismatch\n");
}

void kernel_main(void)
{
        uint32_t magic = mboot_magic;
        uint32_t addr = mboot_addr;
        multiboot_t *mb;
        uint32_t mod_start;
        struct super_block *ram_sb;
        struct super_block *tar_sb;
        struct file *f;
        static uint32_t kernel_stack_for_tss[1024];
        char buf[256];
        ssize_t bytes;
        task_t *shell;
        // 出屎化
        serial_init();
        serial_write("Kernel started\n");

        paging_init(0x00100000, 0x01000000);
        heap_init();
        heap_selftest();
        bdev_selftest();
        vga_init();
        idt_init();

        task_init();
        tss_init();
        tss_set_kernel_stack((uint32_t)(kernel_stack_for_tss + 1024));
        watchdog_init();
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
                        if (mod_start >= 4 * 1024 * 1024)
                        {
                                serial_write("[fs] initrd above 4MB, not directly mapped\n");
                                while (1)
                                        __asm__ volatile("hlt");
                        }
                        ram_sb = ramfs_mount((void *)mod_start);
                        if (ram_sb && ram_sb->s_root)
                        {
                                vfs_root = ram_sb->s_root;
                                serial_write("[fs] ramfs as root\n");
                        }
                }
        }

        tar_sb = tarfs_mount(bdev_lookup("ata0"));
        if (tar_sb)
                vfs_mount("/mnt", tar_sb);

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

                f = vfs_open("/mnt/shell.elf", O_RDONLY);
                if (f)
                {
                        bytes = vfs_read(f, buf, 64);
                        if (bytes > 0)
                                serial_write("[fs] cache warm read ok\n");
                        vfs_close(f);
                        f = vfs_open("/mnt/shell.elf", O_RDONLY);
                        if (f)
                        {
                                bytes = vfs_read(f, buf, 64);
                                if (bytes > 0)
                                        serial_write("[fs] cache re-read ok\n");
                                vfs_close(f);
                        }
                }

                f = vfs_open("/mnt/big.txt", O_RDONLY);
                if (f)
                {
                        char *big = (char *)kmalloc(32768);

                        if (big)
                        {
                                bytes = vfs_read(f, big, 32768);
                                serial_write("[fs] big.txt read ");
                                serial_write_hex(bytes);
                                serial_write(" bytes\n");
                                kfree(big);
                        }
                        vfs_close(f);
                }
        }

        shell = spawn_user_process("/mnt/shell.elf");
        if (!shell)
        {
                serial_write("[boot] spawn shell failed\n");
                while (1)
                        __asm__ volatile("hlt");
        }

        watchdog_register("/mnt/shell.elf", TIER_SYSTEM, shell);
        serial_write("[boot] entering idle loop\n");

        for (;;)
        {
                watchdog_check();
                schedule();
                __asm__ volatile("sti; hlt");
        }
}
