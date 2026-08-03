#include <fs/ramfs.h>
#include <fs/tar.h>
#include <fs/vfs.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/serial.h>
#include <stddef.h>

struct ramfs_inode
{
        uint32_t size;
        uint8_t *data;
};

static struct super_block *ramfs_sb = NULL;

static int ramfs_block_read(void *ctx, uint32_t blk, uint32_t nblk, void *buf)
{
        uint8_t *base = (uint8_t *)ctx;

        memcpy(buf, base + blk * TAR_BLOCK_SIZE, nblk * TAR_BLOCK_SIZE);
        return 0;
}

static struct inode *ramfs_alloc_inode(uint32_t mode)
{
        struct inode *inode;
        struct ramfs_inode *rin;

        inode = vfs_new_inode(mode);
        if (!inode)
                return NULL;

        rin = kmalloc(sizeof(struct ramfs_inode));
        if (!rin)
        {
                kfree(inode);
                return NULL;
        }

        rin->size = 0;
        rin->data = NULL;

        inode->i_private = rin;

        return inode;
}

static ssize_t ramfs_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
        struct ramfs_inode *rin;
        size_t avail;

        if (!filp || !filp->f_dentry || !filp->f_dentry->d_inode)
                return -1;

        rin = (struct ramfs_inode *)filp->f_dentry->d_inode->i_private;
        if (!rin || !rin->data)
                return -1;

        avail = (rin->size > *off) ? (rin->size - *off) : 0;
        if (len > avail)
                len = avail;

        memcpy(buf, rin->data + *off, len);
        *off += len;

        return len;
}

static const struct file_operations ramfs_fops = {
    .read = ramfs_read,
};

static const struct inode_operations ramfs_iops = {
    .lookup = vfs_generic_lookup,
};

static int ramfs_fill_super(struct super_block *sb, void *img)
{
        struct inode *root_inode;
        struct dentry *root_dentry;
        struct tar_entry entry;
        uint32_t blk = 0;

        if (!sb || !img)
                return -1;

        root_inode = ramfs_alloc_inode(S_IFDIR | 0755);
        if (!root_inode)
                return -1;
        root_inode->i_op = &ramfs_iops;

        root_dentry = vfs_new_dentry("/", root_inode, NULL);
        if (!root_dentry)
        {
                kfree(root_inode->i_private);
                kfree(root_inode);
                return -1;
        }
        sb->s_root = root_dentry;

        while (tar_next_entry(ramfs_block_read, img, &blk, &entry) == 0)
        {
                struct inode *file_inode;
                struct ramfs_inode *rin;
                struct dentry *dentry;

                if (entry.type != '0' && entry.type != '\0')
                        continue;

                file_inode = ramfs_alloc_inode(S_IFREG | 0644);
                if (!file_inode)
                        continue;

                rin = (struct ramfs_inode *)file_inode->i_private;
                rin->size = entry.size;
                rin->data = (uint8_t *)img + entry.offset;
                file_inode->i_size = entry.size;
                file_inode->i_fop = &ramfs_fops;

                dentry = vfs_new_dentry(entry.name, file_inode, root_dentry);
                if (!dentry)
                {
                        kfree(file_inode->i_private);
                        kfree(file_inode);
                }
        }

        return 0;
}

struct super_block *ramfs_mount(void *img)
{
        struct super_block *sb;

        if (ramfs_sb)
                return ramfs_sb;

        sb = kmalloc(sizeof(struct super_block));
        if (!sb)
                return NULL;

        memset(sb, 0, sizeof(struct super_block));
        sb->s_fs_info = img;
        sb->s_root = NULL;

        if (ramfs_fill_super(sb, img) != 0)
        {
                kfree(sb);
                return NULL;
        }

        ramfs_sb = sb;
        serial_write("[ramfs] mounted root (tar zero-copy)\n");
        return sb;
}
