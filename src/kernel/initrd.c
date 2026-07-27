#include <kernel/initrd.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <driver/vga.h>
#include <kernel/kstring.h>
static initrd_header_t *initrd_header;
static initrd_file_header_t *file_headers;
static struct inode *root_inode;
static struct inode *dev_inode;
static struct inode *file_inodes;
static int nroot_files;

static ssize_t initrd_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    struct inode *inode = filp->f_dentry->d_inode;
    uint32_t idx = inode->i_ino;
    initrd_file_header_t header;

    if (idx >= nroot_files)
        return -1;
    header = file_headers[idx];
    if (*off >= header.length)
        return 0;
    if (*off + len > header.length)
        len = header.length - *off;
    memcpy(buf, (uint8_t *)(header.offset + (uint32_t)initrd_header) + *off, len);
    *off += len;
    return len;
}

static int initrd_open(struct inode *inode, struct file *filp)
{
        return 0;
}

static int initrd_release(struct inode *inode, struct file *filp)
{
        return 0;
}

/*
 * 修正1：lookup 的第二个参数是 struct dentry *，而不是 const char *
 * 文件名从 dentry->d_name 获取
 */
static struct dentry *initrd_lookup(struct inode *dir, struct dentry *dentry)
{
    const char *name = dentry->d_name;
    struct dentry *child;

    if (dir != root_inode)
        return NULL;

    /* 查找 /dev 目录 */
        if (strcmp(name, "dev") == 0) {
                child = dir->i_children;
                while (child) {
                                if (strcmp(child->d_name, "dev") == 0)
                                        return child;
                                        child = child->d_next;
        }
        return NULL;
    }

    /* 查找普通文件 */
        for (int i = 0; i < nroot_files; i++) {
                if (strcmp(file_headers[i].name, name) == 0) {
            /*
             * 修正2：vfs_new_dentry 的第三个参数是父 dentry（struct dentry *）
             * 不能传 dir（struct inode *），这里直接传 vfs_root
             * 因为我们的文件都直接挂在根目录下
             */
                        struct dentry *newd = vfs_new_dentry(name, &file_inodes[i], vfs_root);
                return newd;
                }
        }
        return NULL;
}

static const struct file_operations initrd_file_ops = {
        .read    = initrd_read,
        .write   = NULL,
        .llseek  = NULL,
        .open    = initrd_open,
        .release = initrd_release,
        .readdir = NULL,
};

/*
 * 修正3：.lookup 的签名现在是 (struct inode *, struct dentry *)
 * 与 initrd_lookup 的定义完全匹配
 */
static const struct inode_operations initrd_dir_ops = {
        .lookup = initrd_lookup,
};

struct dentry *initialise_initrd(uint32_t location)
{
        int i;

        initrd_header = (initrd_header_t *)location;
        file_headers = (initrd_file_header_t *)(location + sizeof(initrd_header_t));
        nroot_files = initrd_header->nfiles;

        root_inode = vfs_new_inode(S_IFDIR);
        if (!root_inode)
                return NULL;
                root_inode->i_op = &initrd_dir_ops;

                dev_inode = vfs_new_inode(S_IFDIR);
                if (dev_inode) {
                        dev_inode->i_op = &initrd_dir_ops;
        }   

        file_inodes = (struct inode *)kmalloc(sizeof(struct inode) * nroot_files);
        if (!file_inodes)
                return NULL;

        for (i = 0; i < nroot_files; i++) {
                file_headers[i].offset += location;
                memset(&file_inodes[i], 0, sizeof(struct inode));
                file_inodes[i].i_ino = i + 2;
                file_inodes[i].i_mode = S_IFREG;
                file_inodes[i].i_size = file_headers[i].length;
                file_inodes[i].i_fop = &initrd_file_ops;
        }

        vfs_mount_root(root_inode);

        if (dev_inode) {
                struct dentry *dev_dentry = vfs_new_dentry("dev", dev_inode, vfs_root);
                if (dev_dentry) {
                        dev_dentry->d_next = root_inode->i_children;
                        root_inode->i_children = dev_dentry;
                }
        }

        for (i = 0; i < nroot_files; i++) {
                struct dentry *d = vfs_new_dentry(file_headers[i].name,
                                          &file_inodes[i], vfs_root);
                if (!d) {
                vga_write("[INITRD] Failed to create dentry for ");
                vga_write(file_headers[i].name);
                vga_write("\n");
                }
        }

    /*
     * 修正4：vga_write_hex 不存在，我们暂时直接输出一个简单的信息
     * 如果你想看文件数量，可以自己实现一个简单的十六进制输出函数
     */
        vga_write("[INITRD] Initialised.\n");

        return vfs_root;
}
