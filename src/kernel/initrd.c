#include <driver/vga.h>
#include <kernel/heap.h>
#include <kernel/initrd.h>
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

        if (idx < 2 || idx - 2 >= (uint32_t)nroot_files)
                return -1;
        header = file_headers[idx - 2];
        if (*off >= header.length)
                return 0;
        if (*off + len > header.length)
                len = header.length - *off;
        memcpy(buf, (uint8_t *)header.offset + *off, len);

        *off += len;
        return len;
}

static int initrd_open(struct inode *inode, struct file *filp)
{
        filp->f_pos = 0;
        return 0;
}

static int initrd_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static struct dentry *initrd_lookup(struct inode *dir, struct dentry *dentry)
{
        const char *name = dentry->d_name;
        struct dentry *child;
        int i;

        if (dir != root_inode)
                return NULL;

        child = dir->i_children;
        while (child)
        {
                if (strcmp(child->d_name, name) == 0)
                        return child;
                child = child->d_next;
        }

        for (i = 0; i < nroot_files; i++)
        {
                if (strcmp(file_headers[i].name, name) == 0)
                        return vfs_new_dentry(name, &file_inodes[i], vfs_root);
        }
        return NULL;
}

static const struct file_operations initrd_file_ops = {
    .read = initrd_read,
    .write = NULL,
    .llseek = NULL,
    .open = initrd_open,
    .release = initrd_release,
    .readdir = NULL,
};

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
        if (dev_inode)
                dev_inode->i_op = &initrd_dir_ops;

        file_inodes = (struct inode *)kmalloc(sizeof(struct inode) * nroot_files);
        if (!file_inodes)
                return NULL;

        for (i = 0; i < nroot_files; i++)
        {
                file_headers[i].offset += location;
                memset(&file_inodes[i], 0, sizeof(struct inode));
                file_inodes[i].i_ino = i + 2;
                file_inodes[i].i_mode = S_IFREG;
                file_inodes[i].i_size = file_headers[i].length;
                file_inodes[i].i_fop = &initrd_file_ops;
        }

        vfs_mount_root(root_inode);

        if (dev_inode)
        {
                struct dentry *dev_dentry = vfs_new_dentry("dev", dev_inode, vfs_root);

                if (dev_dentry)
                {
                        dev_dentry->d_next = root_inode->i_children;
                        root_inode->i_children = dev_dentry;
                }
        }

        for (i = 0; i < nroot_files; i++)
                vfs_new_dentry(file_headers[i].name, &file_inodes[i], vfs_root);

        return vfs_root;
}
