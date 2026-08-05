#include <driver/keyboard.h>
#include <driver/vga.h>
#include <fs/vfs.h>
#include <kernel/caps.h>
#include <kernel/errno.h>
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

typedef int (*syscall_fn)(struct pt_regs *regs);

/* 每个系统调用所需的能力位集合；0 表示 USER 也可直接调用 */
static const uint32_t syscall_caps[] = {
    [SYS_WRITE] = CAP_WRITE,
    [SYS_OPEN] = CAP_READ,
    [SYS_EXIT] = CAP_EXIT,
    [SYS_GETCHAR] = CAP_GETCHAR,
    [SYS_PUTCHAR] = CAP_PUTCHAR,
    [SYS_CLEAR] = CAP_CLEAR,
    [SYS_REBOOT] = CAP_REBOOT,
    [SYS_POWEROFF] = CAP_POWEROFF,
    [SYS_TIER_QUERY] = CAP_TIER_QUERY,
    [SYS_TIER_REQUEST] = CAP_TIER_REQUEST,
    [SYS_FORK] = CAP_FORK,
    [SYS_FORK_WITH_SANDBOX] = CAP_FORK,
    [SYS_EXEC] = CAP_EXEC,
    [SYS_WAIT] = CAP_WAIT,
    [SYS_MLFQ_QUERY] = CAP_MLFQ_QUERY,
    [SYS_YIELD] = CAP_YIELD,
    [SYS_READ] = CAP_READ,
    [SYS_CLOSE] = CAP_READ,
    [SYS_KILL] = CAP_KILL,
    [SYS_SET_TIER] = CAP_SET_TIER,
    [SYS_UNAME] = 0,
    [SYS_LS] = CAP_READ,
    [SYS_SANDBOX_QUERY] = CAP_SANDBOX_QUERY,
};

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

static char exec_argv[MAX_EXEC_ARGS + 1][EXEC_ARG_MAXLEN];

/* 从用户态复制 argv 指针数组及其字符串到内核缓冲 */
static int copy_exec_argv(uint32_t user_argv)
{
        int i;
        int k;

        if (!user_argv)
        {
                exec_argv[0][0] = '\0';
                return 0;
        }
        for (i = 0; i < MAX_EXEC_ARGS; i++)
        {
                uint32_t ptr;
                uint32_t page;
                uint32_t *pd;
                uint32_t *pt;
                uint32_t pd_idx;
                uint32_t pt_idx;

                page = user_argv + (uint32_t)i * 4;
                pd = (uint32_t *)0xFFFFF000;
                pd_idx = page >> 22;
                if (!(pd[pd_idx] & PTE_PRESENT))
                        return -EINVAL;
                pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                pt_idx = (page >> 12) & 0x3FF;
                if (!(pt[pt_idx] & PTE_PRESENT) || !(pt[pt_idx] & PTE_USER))
                        return -EINVAL;
                ptr = *(uint32_t *)page;
                if (!ptr)
                        break;
                if (ptr >= 0xC0000000)
                        return -EINVAL;
                for (k = 0; k < EXEC_ARG_MAXLEN - 1; k++)
                {
                        page = ptr + (uint32_t)k;
                        pd = (uint32_t *)0xFFFFF000;
                        pd_idx = page >> 22;
                        if (!(pd[pd_idx] & PTE_PRESENT))
                                return -EINVAL;
                        pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                        pt_idx = (page >> 12) & 0x3FF;
                        if (!(pt[pt_idx] & PTE_PRESENT) || !(pt[pt_idx] & PTE_USER))
                                return -EINVAL;
                        exec_argv[i][k] = *(char *)page;
                        if (exec_argv[i][k] == '\0')
                                break;
                }
                exec_argv[i][k] = '\0';
        }
        exec_argv[i][0] = '\0';
        return 0;
}

static int sys_exec_from_user(struct pt_regs *regs, uint32_t path_ptr)
{
        static char path[128];
        uint32_t i;

        if (path_ptr >= 0xC0000000)
                return -EINVAL;
        for (i = 0; i < sizeof(path) - 1; i++)
        {
                uint32_t page = path_ptr + i;
                uint32_t *pd = (uint32_t *)0xFFFFF000;
                uint32_t *pt;
                uint32_t pd_idx = page >> 22;
                uint32_t pt_idx = (page >> 12) & 0x3FF;

                if (!(pd[pd_idx] & PTE_PRESENT))
                        return -EINVAL;
                pt = (uint32_t *)(0xFFC00000 + (pd_idx << 12));
                if (!(pt[pt_idx] & PTE_PRESENT) || !(pt[pt_idx] & PTE_USER))
                        return -EINVAL;
                path[i] = *(char *)(path_ptr + i);
                if (path[i] == '\0')
                {
                        if (copy_exec_argv(regs->ecx) != 0)
                                return -EINVAL;
                        return sys_exec(regs, path, exec_argv);
                }
        }
        path[sizeof(path) - 1] = '\0';
        return -EINVAL;
}

/* ---- 系统调用实现 ---- */

static int sys_write_fn(struct pt_regs *regs)
{
        task_t *cur;
        int fd = regs->ebx;
        const void *buf = (const void *)regs->ecx;
        size_t len = (size_t)regs->edx;

        if (fd == 1 || fd == 2)
        {
                if (!user_range_valid((uint32_t)buf, (uint32_t)len))
                        return -EINVAL;
                write_buf(buf, len);
                return 0;
        }

        cur = get_current_task();
        if (!cur || fd < 3 || fd >= FD_MAX || !cur->fd_table[fd])
                return -EINVAL;
        if (!user_range_valid((uint32_t)buf, (uint32_t)len))
                return -EINVAL;
        return vfs_write(cur->fd_table[fd], (const char *)buf, len);
}

/* 从用户态拷贝以 NUL 结尾的路径字符串到内核缓冲 */
static int copy_user_path(uint32_t src, char *dst, uint32_t maxlen)
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

static int sys_open_fn(struct pt_regs *r)
{
        task_t *cur;
        char path[128];
        char resolved[128];
        struct file *f;
        int fd;

        cur = get_current_task();
        if (!cur)
                return -EINVAL;
        if (copy_user_path(r->ebx, path, sizeof(path)) != 0)
                return -EINVAL;

        resolve_path(cur, path, resolved, sizeof(resolved));

        f = vfs_open(resolved, r->ecx);
        if (!f && (r->ecx & O_CREAT))
                f = vfs_create(resolved, (r->edx & 0xFFFF) | S_IFREG);
        if (!f)
                return -ENOENT;

        for (fd = 3; fd < FD_MAX; fd++)
        {
                if (!cur->fd_table[fd])
                        break;
        }
        if (fd >= FD_MAX)
        {
                vfs_close(f);
                return -ENOMEM;
        }

        cur->fd_table[fd] = f;
        return fd;
}

static int sys_read_fn(struct pt_regs *r)
{
        task_t *cur;
        int fd = (int)r->ebx;
        void *buf = (void *)r->ecx;
        size_t len = (size_t)r->edx;

        cur = get_current_task();
        if (!cur)
                return -EINVAL;

        if (fd == 0)
        {
                char c;

                if (!user_range_valid((uint32_t)buf, 1))
                        return -EINVAL;
                c = getchar();
                if (c == 0)
                        return 0;
                *(char *)buf = c;
                return 1;
        }

        if (fd < 3 || fd >= FD_MAX || !cur->fd_table[fd])
                return -EINVAL;
        if (!user_range_valid((uint32_t)buf, (uint32_t)len))
                return -EINVAL;
        return vfs_read(cur->fd_table[fd], (char *)buf, len);
}

static int sys_close_fn(struct pt_regs *r)
{
        task_t *cur;
        int fd = (int)r->ebx;
        int ret;

        cur = get_current_task();
        if (!cur || fd < 3 || fd >= FD_MAX || !cur->fd_table[fd])
                return -EINVAL;
        ret = vfs_close(cur->fd_table[fd]);
        cur->fd_table[fd] = NULL;
        return ret;
}

/* 列出目录内容：每次调用返回 "name\n" 序列写入用户 buf，返回字节数 */
static int sys_ls_fn(struct pt_regs *r)
{
        task_t *cur;
        char path[128];
        char resolved[128];
        struct dentry *dentry;
        struct inode *inode;
        struct dentry *child;
        uint32_t ubuf = (uint32_t)r->ecx;
        uint32_t buflen = (uint32_t)r->edx;
        uint32_t off = 0;

        cur = get_current_task();
        if (!cur)
                return -EINVAL;
        if (copy_user_path(r->ebx, path, sizeof(path)) != 0)
                return -EINVAL;

        resolve_path(cur, path, resolved, sizeof(resolved));

        dentry = vfs_lookup(resolved);
        if (!dentry)
                return -ENOENT;
        inode = dentry->d_inode;
        if (!inode)
                return -ENOENT;
        if (!(inode->i_mode & S_IFDIR))
                return -ENOTDIR;

        for (child = inode->i_children; child; child = child->d_next)
        {
                uint32_t n = strlen(child->d_name);
                uint32_t i;

                if (off + n + 1 > buflen)
                        break;
                if (!user_range_valid(ubuf + off, n + 1))
                        return -EINVAL;
                for (i = 0; i < n; i++)
                        *(char *)(ubuf + off + i) = child->d_name[i];
                *(char *)(ubuf + off + n) = '\n';
                off += n + 1;
        }

        return (int)off;
}

static int sys_exit_fn(struct pt_regs *r)
{
        task_exit((int)r->ebx);
        return 0;
}

static int sys_getchar_fn(struct pt_regs *r)
{
        (void)r;
        return getchar();
}

static int sys_putchar_fn(struct pt_regs *r)
{
        put_char((char)r->ebx);
        return 0;
}

static int sys_clear_fn(struct pt_regs *r)
{
        (void)r;
        vga_clear();
        return 0;
}

static int sys_reboot_fn(struct pt_regs *r)
{
        (void)r;
        outb(0x64, 0xFE);
        return 0;
}

static int sys_poweroff_fn(struct pt_regs *r)
{
        (void)r;
        outw(0x604, 0x2000);
        return 0;
}

static int sys_tier_query_fn(struct pt_regs *r)
{
        task_t *cur;

        (void)r;
        cur = get_current_task();
        return cur ? (int)cur->tier : -1;
}

static int sys_tier_request_fn(struct pt_regs *r)
{
        task_t *cur;

        if (r->ebx > TIER_CRITICAL)
                return -1;
        cur = get_current_task();
        if (!cur)
                return -1;
        task_set_tier(cur, (tier_t)r->ebx);
        return (int)cur->tier;
}

static int sys_fork_fn(struct pt_regs *r)
{
        return sys_fork(r, NULL);
}

static int sys_fork_with_sandbox_fn(struct pt_regs *r)
{
        if (copy_sandbox_config((uint32_t)r->ebx) != 0)
                return -1;
        return sys_fork(r, &kernel_sb);
}

static int sys_exec_fn(struct pt_regs *r)
{
        return sys_exec_from_user(r, (uint32_t)r->ebx);
}

static int sys_wait_fn(struct pt_regs *r)
{
        return sys_wait((int)r->ebx);
}

static int sys_mlfq_query_fn(struct pt_regs *r)
{
        task_t *cur;

        (void)r;
        cur = get_current_task();
        return cur ? cur->mlfq_level : -1;
}

static int sys_yield_fn(struct pt_regs *r)
{
        (void)r;
        yield();
        return 0;
}

/* 沙盒观测与控制 */

static int sys_kill_fn(struct pt_regs *r)
{
        task_t *cur;
        task_t *t;

        cur = get_current_task();
        if (!cur)
                return -1;
        t = task_find_child(cur, (int)r->ebx);
        if (!t || t->state == TASK_EXITED)
                return -1;
        t->kill_pending = 1;
        serial_write("[kill] pid ");
        serial_write_hex(t->pid);
        serial_write(" (self ");
        serial_write_hex(cur->pid);
        serial_write(") marked\n");
        return 0;
}

static int sys_set_tier_fn(struct pt_regs *r)
{
        task_t *cur;

        if (r->ebx > TIER_CRITICAL)
                return -1;
        cur = get_current_task();
        if (!cur)
                return -1;
        task_set_tier(cur, (tier_t)r->ebx);
        cur->tier_override = (int)r->ebx;
        return (int)cur->tier;
}

static int sys_uname_fn(struct pt_regs *r)
{
        const char *name = "PokuOS 0.1";
        uint32_t len;

        len = strlen(name) + 1;
        if (r->ebx >= 0xC0000000)
                return -1;
        if (!user_range_valid(r->ebx, len))
                return -1;
        memcpy((void *)r->ebx, name, len);
        return (int)len;
}

static int sys_sandbox_query_fn(struct pt_regs *r)
{
        task_t *cur;
        struct sandbox_status st;
        uint32_t *mem;

        cur = get_current_task();
        if (!cur || r->ebx >= 0xC0000000)
                return -1;
        if (!user_range_valid(r->ebx, sizeof(struct sandbox_status)))
                return -1;

        mem = (uint32_t *)&cur->mem_limit;
        st.pid = cur->pid;
        st.caps = cur->caps;
        st.tier = (uint32_t)cur->tier;
        st.mem_limit_lo = mem[0];
        st.mem_limit_hi = mem[1];
        st.cpu_quota = cur->cpu_quota;
        st.pages_charged = cur->pages_charged;
        st.quota_used_ticks = cur->quota_used_ticks;
        memset(st.root_path, 0, sizeof(st.root_path));
        strcpy(st.root_path, cur->root_path);
        memcpy((void *)r->ebx, &st, sizeof(st));
        return 0;
}

static const syscall_fn syscall_table[] = {
    [SYS_WRITE] = sys_write_fn,
    [SYS_OPEN] = sys_open_fn,
    [SYS_EXIT] = sys_exit_fn,
    [SYS_GETCHAR] = sys_getchar_fn,
    [SYS_PUTCHAR] = sys_putchar_fn,
    [SYS_CLEAR] = sys_clear_fn,
    [SYS_REBOOT] = sys_reboot_fn,
    [SYS_POWEROFF] = sys_poweroff_fn,
    [SYS_TIER_QUERY] = sys_tier_query_fn,
    [SYS_TIER_REQUEST] = sys_tier_request_fn,
    [SYS_FORK] = sys_fork_fn,
    [SYS_EXEC] = sys_exec_fn,
    [SYS_WAIT] = sys_wait_fn,
    [SYS_FORK_WITH_SANDBOX] = sys_fork_with_sandbox_fn,
    [SYS_MLFQ_QUERY] = sys_mlfq_query_fn,
    [SYS_YIELD] = sys_yield_fn,
    [SYS_READ] = sys_read_fn,
    [SYS_CLOSE] = sys_close_fn,
    [SYS_KILL] = sys_kill_fn,
    [SYS_SET_TIER] = sys_set_tier_fn,
    [SYS_UNAME] = sys_uname_fn,
    [SYS_LS] = sys_ls_fn,
    [SYS_SANDBOX_QUERY] = sys_sandbox_query_fn,
};

#define NUM_SYSCALLS (sizeof(syscall_table) / sizeof(syscall_table[0]))

int syscall_handler(struct pt_regs *regs)
{
        uint32_t num;
        uint32_t need;
        task_t *cur;

        if (!regs)
                return -1;

        num = regs->eax;
        if (num >= NUM_SYSCALLS || !syscall_table[num])
                return -1;

        cur = get_current_task();
        if (!cur)
                return -1;

        /* 能力门控：无权限的系统调用一律拒绝 */
        need = syscall_caps[num];
        if (need && (cur->caps & need) != need)
                return -EPERM;

        return syscall_table[num](regs);
}

void syscall_init(void)
{
}
