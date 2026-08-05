#include <fs/vfs.h>
#include <kernel/elf.h>
#include <kernel/errno.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/paging.h>
#include <kernel/process.h>
#include <kernel/pt_regs.h>
#include <kernel/sandbox.h>
#include <kernel/sched.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <user/usermode.h>
#include <vm/vm.h>

#define USER_STACK_VIRT 0xBFFFF000
#define MAX_USER_PAGES 256
#define EXEC_BUF_SIZE 65536

extern void fork_return(void);
extern uint32_t page_directory[];

struct page_ref
{
        uint32_t vaddr;
        uint32_t phys;
};

static void user_process_entry(void)
{
        task_t *t;

        t = current_task;
        if (!t)
                return;
        switch_to_user(t->user_entry, t->user_stack);
        for (;;)
                __asm__ volatile("hlt");
}

/* 释放当前（cr3 已指向）PD 的用户地址空间，然后切到 target_cr3 并释放该 PD 帧 */
static void teardown_current_pd(uint32_t pd_phys, uint32_t target_cr3)
{
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t i, j;

        for (i = 1; i < 768; i++)
        {
                uint32_t *pt;

                if (!(pd[i] & PTE_PRESENT))
                        continue;
                pt = (uint32_t *)(0xFFC00000 + (i << 12));
                for (j = 0; j < 1024; j++)
                {
                        if (pt[j] & PTE_PRESENT)
                                free_page_frame(pt[j] & 0xFFFFF000);
                }
                free_page_frame(pd[i] & 0xFFFFF000);
                pd[i] = 0;
        }
        load_cr3(target_cr3);
        free_page_frame(pd_phys);
}

void free_task_address_space(task_t *t)
{
        if (!t)
                return;
        teardown_current_pd(t->cr3, (uint32_t)page_directory);
}

static int copy_user_string(uint32_t src, char *dst, uint32_t maxlen)
{
        uint32_t i;

        if (src >= 0xC0000000)
                return -1;
        for (i = 0; i < maxlen - 1; i++)
        {
                uint32_t page = src + i;
                uint32_t *pd = (uint32_t *)0xFFFFF000;
                uint32_t *pt;
                uint32_t pd_idx = page >> 22;
                uint32_t pt_idx = (page >> 12) & 0x3FF;

                if (!(pd[pd_idx] & PTE_PRESENT))
                        return -1;
                pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                if (!(pt[pt_idx] & PTE_PRESENT) || !(pt[pt_idx] & PTE_USER))
                        return -1;
                dst[i] = *(char *)(src + i);
                if (dst[i] == '\0')
                        return 0;
        }
        dst[maxlen - 1] = '\0';
        return 0;
}

static void task_root_path(task_t *t, const char *src)
{
        if (!src)
        {
                t->root_path[0] = '\0';
                return;
        }
        if (copy_user_string((uint32_t)src, t->root_path,
                             sizeof(t->root_path)) != 0)
        {
                t->root_path[0] = '\0';
        }
}

/* child 路径必须是 parent 前缀路径的子路径（等于或位于其下） */
static int path_is_within(const char *child, const char *parent)
{
        size_t plen;

        if (!child || !parent)
                return 0;
        plen = strlen(parent);
        if (strncmp(child, parent, plen) != 0)
                return 0;
        return child[plen] == '\0' || child[plen] == '/';
}

/* 将 chroot 前缀应用到路径 */
void resolve_path(task_t *t, const char *path, char *buf, int buflen)
{
        size_t rlen;
        size_t plen;
        int i;

        if (!t || t->root_path[0] == '\0' || strcmp(t->root_path, "/") == 0)
        {
                strcpy(buf, path);
                return;
        }
        rlen = strlen(t->root_path);
        plen = strlen(path);
        if (rlen + plen + 2 > (size_t)buflen)
        {
                strcpy(buf, path);
                return;
        }
        for (i = 0; i < (int)rlen; i++)
                buf[i] = t->root_path[i];
        if (buf[rlen - 1] != '/')
        {
                buf[rlen] = '/';
                rlen++;
        }
        for (i = 0; i < (int)plen; i++)
                buf[rlen + i] = path[i];
        buf[rlen + plen] = '\0';
}

static uint32_t count_user_pages(void)
{
        uint32_t *pd = (uint32_t *)0xFFFFF000;
        uint32_t i, j, n = 0;

        for (i = 1; i < 768; i++)
        {
                uint32_t *pt;

                if (!(pd[i] & PTE_PRESENT))
                        continue;
                pt = (uint32_t *)(0xFFC00000 + (i << 12));
                for (j = 0; j < 1024; j++)
                {
                        if (pt[j] & PTE_PRESENT)
                                n++;
                }
        }
        return n;
}

int sys_fork(struct pt_regs *regs, const struct fork_sandbox_config *sb)
{
        task_t *parent = current_task;
        task_t *child;
        uint32_t *pd;
        uint32_t child_pd;
        uint32_t old_cr3;
        struct page_ref pages[MAX_USER_PAGES];
        uint32_t npages = 0;
        uint32_t i, j;
        uint32_t *sp;
        uint32_t *pusha;
        uint32_t *intframe;
        uint32_t user_eip, user_cs, user_eflags, user_esp, user_ss;
        uint32_t limit_pages;

        if (!parent || !regs)
                return -1;

        /* 收集父进程用户页（当前 cr3 = 父 PD） */
        pd = (uint32_t *)0xFFFFF000;
        for (i = 1; i < 768; i++)
        {
                uint32_t *pt;

                if (!(pd[i] & PTE_PRESENT))
                        continue;
                pt = (uint32_t *)(0xFFC00000 + (i << 12));
                for (j = 0; j < 1024; j++)
                {
                        if (pt[j] & PTE_PRESENT)
                        {
                                if (npages >= MAX_USER_PAGES)
                                        return -1;
                                pages[npages].vaddr = (i << 22) | (j << 12);
                                pages[npages].phys = pt[j] & 0xFFFFF000;
                                npages++;
                        }
                }
        }

        child = create_task_slot();
        if (!child)
                return -1;

        child->priority = parent->priority;
        child->timeslice = TASK_TIMESLICE;
        child->state = TASK_READY;
        child->mlfq_level = 0;
        child->parent = parent;
        child->map = NULL;
        child->wd_managed = 0;

        /* 沙盒继承/覆盖：受限父只能产生同等或更受限的子进程，
         * 不允许通过 fork_with_sandbox 扩大 root_path / 等级 / 配额。 */
        if (sb && sb->tier_override)
        {
                if (sb->tier_override < 0 || sb->tier_override > TIER_CRITICAL)
                {
                        free_task_slot(child);
                        return -EINVAL;
                }
                if (!(parent->caps & CAP_SET_TIER))
                {
                        free_task_slot(child);
                        return -EPERM;
                }
                child->tier_override = sb->tier_override;
                child->tier = (tier_t)sb->tier_override;
        }
        else
        {
                child->tier_override = parent->tier_override;
                child->tier = parent->tier;
        }
        child->caps = (sb && sb->caps_allow) ? (parent->caps & sb->caps_allow)
                                             : parent->caps;
        if (sb && sb->mem_limit)
        {
                child->mem_limit = sb->mem_limit;
                if (parent->mem_limit && child->mem_limit > parent->mem_limit)
                        child->mem_limit = parent->mem_limit;
        }
        else
        {
                child->mem_limit = parent->mem_limit;
        }
        if (sb && sb->cpu_quota)
        {
                child->cpu_quota = (sb->cpu_quota > 100) ? 100 : sb->cpu_quota;
                if (parent->cpu_quota && child->cpu_quota > parent->cpu_quota)
                        child->cpu_quota = parent->cpu_quota;
        }
        else
        {
                child->cpu_quota = parent->cpu_quota;
        }
        if (sb && sb->root_path)
        {
                char sub_root[64];

                memset(sub_root, 0, sizeof(sub_root));
                if (copy_user_string((uint32_t)sb->root_path, sub_root,
                                     sizeof(sub_root)) != 0)
                {
                        free_task_slot(child);
                        return -EINVAL;
                }
                if (parent->root_path[0] != '\0' &&
                    !path_is_within(sub_root, parent->root_path))
                {
                        free_task_slot(child);
                        return -EPERM;
                }
                strcpy(child->root_path, sub_root);
        }
        else
        {
                strcpy(child->root_path, parent->root_path);
        }

        limit_pages = child->mem_limit ? (uint32_t)(child->mem_limit / 4096) : 0;
        if (limit_pages && npages > limit_pages)
        {
                free_task_slot(child);
                serial_write("[fork] mem_limit rejected pid=");
                serial_write_hex(child->pid);
                serial_write("\n");
                return -1;
        }

        child_pd = paging_create_task_pd();
        if (!child_pd)
        {
                free_task_slot(child);
                return -1;
        }

        old_cr3 = parent->cr3;
        load_cr3(child_pd);

        for (i = 0; i < npages; i++)
        {
                uint32_t new_phys;

                new_phys = alloc_page_frame();
                if (!new_phys)
                {
                        teardown_current_pd(child_pd, old_cr3);
                        free_task_slot(child);
                        serial_write("[fork] oom\n");
                        return -1;
                }
                map_page((void *)pages[i].vaddr, (void *)new_phys,
                         PTE_PRESENT | PTE_RW | PTE_USER);
                vm_copy_phys(pages[i].phys, new_phys);
        }

        child->cr3 = child_pd;
        child->user_entry = parent->user_entry;
        child->user_stack = parent->user_stack;
        child->pages_charged = npages;
        load_cr3(old_cr3);

        /* 从父进程 syscall 帧构造子进程内核栈：
         * [fork_return][pusha(eax=0, esp 字段=&intframe)][int0x80 帧] */
        intframe = (uint32_t *)regs->esp;
        user_eip = intframe[0];
        user_cs = intframe[1];
        user_eflags = intframe[2];
        user_esp = intframe[3];
        user_ss = intframe[4];

        sp = (uint32_t *)(child->stack + STACK_SIZE);
        *--sp = user_ss;
        *--sp = user_esp;
        *--sp = user_eflags;
        *--sp = user_cs;
        *--sp = user_eip;
        intframe = sp;
        sp -= 8;
        pusha = sp;
        pusha[0] = regs->edi;
        pusha[1] = regs->esi;
        pusha[2] = regs->ebp;
        pusha[3] = (uint32_t)intframe;
        pusha[4] = regs->ebx;
        pusha[5] = regs->edx;
        pusha[6] = regs->ecx;
        pusha[7] = 0;
        *--sp = (uint32_t)fork_return;
        child->esp = (uint32_t)sp;
        child->ebp = child->esp;

        enqueue_task(child);

        return child->pid;
}

static void free_built_pd(uint32_t pd_phys, uint32_t target_cr3)
{
        teardown_current_pd(pd_phys, target_cr3);
}

/* 在用户栈顶构造 argc/argv：esp -> [argc][argv[]][NULL][字符串区] */
static uint32_t build_arg_stack(char argv[][EXEC_ARG_MAXLEN])
{
        uint32_t str_addr[MAX_EXEC_ARGS];
        char *sp;
        int argc;
        int i;

        sp = (char *)(USER_STACK_VIRT + 4096);
        argc = 0;
        while (argc < MAX_EXEC_ARGS && argv[argc][0])
        {
                uint32_t n = strlen(argv[argc]) + 1;

                sp -= n;
                memcpy(sp, argv[argc], n);
                str_addr[argc] = (uint32_t)sp;
                argc++;
        }
        sp = (char *)((uint32_t)sp & ~3u);
        sp -= (argc + 1) * 4;
        for (i = 0; i < argc; i++)
                ((uint32_t *)sp)[i] = str_addr[i];
        ((uint32_t *)sp)[argc] = 0;
        sp -= 4;
        *(uint32_t *)sp = (uint32_t)argc;
        return (uint32_t)sp;
}

int sys_exec(struct pt_regs *regs, const char *path, char argv[][EXEC_ARG_MAXLEN])
{
        task_t *t = current_task;
        struct file *f;
        uint8_t *elf_data;
        ssize_t bytes;
        uint32_t entry;
        uint32_t new_pd;
        uint32_t old_pd;
        uint32_t user_stack_phys;
        uint32_t *p;
        char resolved[128];

        if (!t || !regs || !path)
                return -EINVAL;

        resolve_path(t, path, resolved, sizeof(resolved));

        f = vfs_open(resolved, O_RDONLY);
        if (!f)
                return -ENOENT;
        elf_data = (uint8_t *)kmalloc(EXEC_BUF_SIZE);
        if (!elf_data)
        {
                vfs_close(f);
                return -ENOMEM;
        }
        bytes = vfs_read(f, (char *)elf_data, EXEC_BUF_SIZE - 1);
        vfs_close(f);
        if (bytes <= 0)
        {
                kfree(elf_data);
                return -EIO;
        }

        new_pd = paging_create_task_pd();
        if (!new_pd)
        {
                kfree(elf_data);
                return -ENOMEM;
        }

        load_cr3(new_pd);

        if (elf_load(elf_data, &entry, resolved) != 0)
        {
                /* 当前 cr3 仍是 new_pd，由 free_built_pd 释放并切回旧 PD */
                free_built_pd(new_pd, t->cr3);
                kfree(elf_data);
                return -ENOEXEC;
        }

        user_stack_phys = alloc_page_frame();
        if (!user_stack_phys)
        {
                free_built_pd(new_pd, t->cr3);
                kfree(elf_data);
                return -ENOMEM;
        }
        map_page((void *)USER_STACK_VIRT, (void *)user_stack_phys,
                 PTE_PRESENT | PTE_RW | PTE_USER);

        /* 释放旧地址空间（先切回旧 PD，再释放其帧与 PD 帧） */
        old_pd = t->cr3;
        load_cr3(old_pd);
        free_task_address_space(t);
        load_cr3(new_pd);

        t->cr3 = new_pd;
        t->user_entry = entry;
        t->user_stack = build_arg_stack(argv);
        t->map = NULL;
        t->pages_charged = count_user_pages();

        /* 等级：沙盒覆盖则保留，否则按 ELF 重新判定 */
        if (t->tier_override < 0)
        {
                tier_t nt = elf_assign_tier(resolved);

                if (nt != t->tier)
                        task_set_tier(t, nt);
        }

        /* 重建当前 syscall 栈顶：返回时经 fork_return 直达新入口 */
        p = (uint32_t *)regs;
        p[-2] = (uint32_t)fork_return;
        p[-1] = 0;
        p[0] = 0;
        p[1] = 0;
        p[2] = (uint32_t)&p[7];
        p[3] = 0;
        p[4] = 0;
        p[5] = 0;
        p[6] = 0;
        p[7] = entry;
        p[8] = 0x1B;
        p[9] = 0x202;
        p[10] = t->user_stack;
        p[11] = 0x23;
        kfree(elf_data);
        return 0;
}

int sys_wait(int pid)
{
        task_t *t = current_task;
        task_t *child;
        int code;
        int child_pid;

        if (!t)
                return -1;

        for (;;)
        {
                child = task_find_child(t, pid);
                if (!child)
                        return -1;
                if (child->state == TASK_EXITED)
                        break;
                t->state = TASK_WAITING;
                dequeue_task(t);
                schedule();
        }

        child_pid = child->pid;
        code = child->exit_code;
        free_task_slot(child);

        return (child_pid << 8) | (code & 0xFF);
}

task_t *spawn_user_process(const char *path)
{
        struct file *f;
        uint8_t *elf_data;
        ssize_t bytes;
        uint32_t entry;
        uint32_t task_pd;
        uint32_t user_stack_phys;
        uint32_t old_cr3;
        task_t *t;
        tier_t assigned;

        if (!path)
                return NULL;

        f = vfs_open(path, O_RDONLY);
        if (!f)
        {
                serial_write("[spawn] open failed: ");
                serial_write(path);
                serial_write("\n");
                return NULL;
        }

        elf_data = (uint8_t *)kmalloc(EXEC_BUF_SIZE);
        if (!elf_data)
        {
                vfs_close(f);
                serial_write("[spawn] no memory for ELF buffer\n");
                return NULL;
        }

        bytes = vfs_read(f, (char *)elf_data, EXEC_BUF_SIZE - 1);
        vfs_close(f);
        if (bytes <= 0)
        {
                kfree(elf_data);
                serial_write("[spawn] read failed: ");
                serial_write(path);
                serial_write("\n");
                return NULL;
        }

        task_pd = paging_create_task_pd();
        if (!task_pd)
        {
                kfree(elf_data);
                serial_write("[spawn] no memory for task pd\n");
                return NULL;
        }

        __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
        load_cr3(task_pd);

        if (elf_load(elf_data, &entry, path) != 0)
        {
                load_cr3(old_cr3);
                kfree(elf_data);
                serial_write("[spawn] ELF load failed: ");
                serial_write(path);
                serial_write("\n");
                return NULL;
        }

        user_stack_phys = alloc_page_frame();
        if (!user_stack_phys)
        {
                load_cr3(old_cr3);
                kfree(elf_data);
                serial_write("[spawn] no memory for user stack\n");
                return NULL;
        }
        map_page((void *)USER_STACK_VIRT,
                 (void *)user_stack_phys,
                 PTE_PRESENT | PTE_RW | PTE_USER);

        /* 空参数帧：crt0 从 [esp] 读 argc、[esp+4] 读 argv */
        *((uint32_t *)(USER_STACK_VIRT + 4096 - 4)) = 0;
        *((uint32_t *)(USER_STACK_VIRT + 4096 - 8)) = 0;

        t = create_task(user_process_entry, TASK_PRIO_NORMAL);
        if (!t)
        {
                load_cr3(old_cr3);
                kfree(elf_data);
                serial_write("[spawn] no task slot\n");
                return NULL;
        }

        t->map = vm_map_create();
        if (t->map)
                vm_protect_readonly(t->map);

        /* cr3 仍指向 task_pd，统计新地址空间的用户页数 */
        t->pages_charged = count_user_pages();

        load_cr3(old_cr3);

        t->cr3 = task_pd;
        t->user_entry = entry;
        t->user_stack = USER_STACK_VIRT + 4096 - 8;

        assigned = elf_assign_tier(path);
        t->tier = assigned;
        t->caps = (assigned == TIER_SYSTEM) ? CAP_SYSTEM_DEFAULT : CAP_USER_DEFAULT;

        enqueue_task(t);

        kfree(elf_data);
        return t;
}
