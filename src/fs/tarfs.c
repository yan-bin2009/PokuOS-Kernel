#include <fs/bdev.h>
#include <fs/page_cache.h>
#include <fs/tar.h>
#include <fs/tarfs.h>
#include <fs/vfs.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/serial.h>
#include <stddef.h>

#define TARFS_READAHEAD 4

struct tarfs_inode
{
        uint32_t offset;
        uint32_t size;
};

struct tarfs_file
{
        uint32_t last_pgoff;
};

static struct block_device *tarfs_bdev = NULL;

static int tarfs_block_read(void *ctx, uint32_t blk, uint32_t nblk, void *buf)
{
        struct block_device *dev = (struct block_device *)ctx;

        return bdev_read(dev, blk, buf, nblk, TIER_KERNEL);
}

static int tarfs_readpage(struct inode *inode, uint32_t pgoff, void *buf,
                          tier_t tier)
{
        struct tarfs_inode *tin;
        uint32_t start;
        uint32_t count;
        uint32_t nsec;

        if (!inode || !buf)
                return -1;

        tin = (struct tarfs_inode *)inode->i_private;
        if (!tin)
                return -1;

        start = pgoff * 4096;
        if (start >= tin->size)
                return -1;

        count = (start + 4096 > tin->size) ? (tin->size - start) : 4096;
        nsec = (count + 511) / 512;

        if (bdev_read(tarfs_bdev, (tin->offset + start) / 512, buf, nsec, tier) != 0)
                return -1;

        return count;
}

static ssize_t tarfs_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
        struct inode *inode;
        struct tarfs_inode *tin;
        struct tarfs_file *tfile;
        size_t avail;
        uint32_t pgoff;
        uint32_t pgoff_end;
        size_t total;
        uint8_t page[4096];
        int ret;
        uint32_t i;

        if (!filp || !filp->f_dentry || !filp->f_dentry->d_inode)
                return -1;

        inode = filp->f_dentry->d_inode;
        tin = (struct tarfs_inode *)inode->i_private;
        if (!tin)
                return -1;

        tfile = (struct tarfs_file *)filp->f_private;

        avail = (tin->size > *off) ? (tin->size - *off) : 0;
        if (len > avail)
                len = avail;
        if (len == 0)
                return 0;

        total = 0;
        pgoff = *off / 4096;
        pgoff_end = (*off + len + 4095) / 4096;

        while (pgoff < pgoff_end)
        {
                uint32_t page_off;
                uint32_t page_len;

                ret = readpage(inode, pgoff, page, TIER_KERNEL);
                if (ret < 0)
                        break;

                page_off = (pgoff == *off / 4096) ? (*off % 4096) : 0;
                page_len = 4096 - page_off;
                if (page_len > len - total)
                        page_len = len - total;

                memcpy(buf + total, page + page_off, page_len);
                total += page_len;

                if (tfile)
                {
                        if (pgoff == tfile->last_pgoff + 1 && pgoff + 1 < pgoff_end)
                        {
                                uint8_t ra[4096];

                                for (i = 1; i <= TARFS_READAHEAD &&
                                            pgoff + i < pgoff_end;
                                     i++)
                                        readpage(inode, pgoff + i, ra, TIER_KERNEL);
                                serial_write("[tarfs] readahead ");
                                serial_write_hex(TARFS_READAHEAD);
                                serial_write(" pages\n");
                        }
                        tfile->last_pgoff = pgoff;
                }

                pgoff++;
        }

        *off += total;
        return total;
}

static int tarfs_open(struct inode *inode, struct file *filp)
{
        (void)inode;

        filp->f_private = kmalloc(sizeof(struct tarfs_file));
        if (!filp->f_private)
                return -1;
        ((struct tarfs_file *)filp->f_private)->last_pgoff = 0;
        return 0;
}

static int tarfs_release(struct inode *inode, struct file *filp)
{
        (void)inode;

        if (filp->f_private)
        {
                kfree(filp->f_private);
                filp->f_private = NULL;
        }
        return 0;
}

static const struct file_operations tarfs_fops = {
    .read = tarfs_read,
    .open = tarfs_open,
    .release = tarfs_release,
};

static const struct inode_operations tarfs_iops = {
    .lookup = vfs_generic_lookup,
};

static const struct super_operations tarfs_sops = {
    .readpage = tarfs_readpage,
};

static int tarfs_fill_super(struct super_block *sb, struct block_device *dev)
{
        struct inode *root_inode;
        struct dentry *root_dentry;
        struct tar_entry entry;
        uint32_t blk = 0;
        uint32_t file_count = 0;

        if (!sb || !dev)
                return -1;

        tarfs_bdev = dev;
        sb->s_op = &tarfs_sops;

        root_inode = vfs_new_inode(S_IFDIR | 0755);
        if (!root_inode)
                return -1;
        root_inode->i_sb = sb;
        root_inode->i_op = &tarfs_iops;

        root_dentry = vfs_new_dentry("/", root_inode, NULL);
        if (!root_dentry)
        {
                kfree(root_inode);
                return -1;
        }
        sb->s_root = root_dentry;

        while (tar_next_entry(tarfs_block_read, dev, &blk, &entry) == 0)
        {
                struct inode *file_inode;
                struct tarfs_inode *tin;
                struct dentry *dentry;

                if (entry.type != '0' && entry.type != '\0')
                        continue;

                file_inode = vfs_new_inode(S_IFREG | 0644);
                if (!file_inode)
                        break;
                file_inode->i_sb = sb;
                file_inode->i_size = entry.size;

                tin = kmalloc(sizeof(struct tarfs_inode));
                if (!tin)
                {
                        kfree(file_inode);
                        break;
                }
                tin->offset = entry.offset;
                tin->size = entry.size;
                file_inode->i_private = tin;
                file_inode->i_fop = &tarfs_fops;

                dentry = vfs_new_dentry(entry.name, file_inode, root_dentry);
                if (!dentry)
                {
                        kfree(tin);
                        kfree(file_inode);
                }
                else
                {
                        file_count++;
                }
        }

        serial_write("[tarfs] mounted, files=");
        serial_write_hex(file_count);
        serial_write("\n");

        return 0;
}

struct super_block *tarfs_mount(struct block_device *dev)
{
        struct super_block *sb;

        if (!dev)
                return NULL;

        sb = kmalloc(sizeof(struct super_block));
        if (!sb)
                return NULL;

        memset(sb, 0, sizeof(struct super_block));
        sb->s_fs_info = dev;
        sb->s_root = NULL;

        if (tarfs_fill_super(sb, dev) != 0)
        {
                kfree(sb);
                return NULL;
        }

        return sb;
}
