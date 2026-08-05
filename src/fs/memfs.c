#include <fs/memfs.h>
#include <fs/vfs.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/serial.h>
#include <stddef.h>

/* memfs: 内存中的可写文件系统。文件内容按需在堆上分配，
 * 仅存在于运行期内（重启即丢失）。挂载在 /home 供用户程序保存。 */

struct memfs_inode
{
        uint32_t size;
        uint32_t capacity;
        uint8_t *data;
};

static struct super_block *memfs_sb = NULL;

static const struct file_operations memfs_fops;

static struct inode *memfs_alloc_inode(uint32_t mode)
{
        struct inode *inode;
        struct memfs_inode *min;

        inode = vfs_new_inode(mode);
        if (!inode)
                return NULL;

        min = kmalloc(sizeof(struct memfs_inode));
        if (!min)
        {
                kfree(inode);
                return NULL;
        }

        min->size = 0;
        min->capacity = 0;
        min->data = NULL;

        inode->i_private = min;
        inode->i_fop = NULL;

        return inode;
}

static int memfs_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
        struct inode *inode;

        (void)dir;

        inode = memfs_alloc_inode(mode);
        if (!inode)
                return -1;

        inode->i_fop = &memfs_fops;
        dentry->d_inode = inode;

        return 0;
}

static ssize_t memfs_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
        struct memfs_inode *min;
        size_t avail;

        if (!filp || !filp->f_dentry || !filp->f_dentry->d_inode)
                return -1;

        min = (struct memfs_inode *)filp->f_dentry->d_inode->i_private;
        if (!min || !min->data)
                return -1;

        avail = (min->size > *off) ? (min->size - *off) : 0;
        if (len > avail)
                len = avail;

        memcpy(buf, min->data + *off, len);
        *off += len;

        return len;
}

static ssize_t memfs_write(struct file *filp, const char *buf, size_t len,
                           loff_t *off)
{
        struct memfs_inode *min;
        uint32_t need;
        uint32_t cap;

        if (!filp || !filp->f_dentry || !filp->f_dentry->d_inode)
                return -1;

        min = (struct memfs_inode *)filp->f_dentry->d_inode->i_private;
        if (!min)
                return -1;

        need = (uint32_t)(*off) + (uint32_t)len;
        if (need > min->capacity)
        {
                uint8_t *nd;

                cap = min->capacity ? min->capacity : 64;
                while (cap < need)
                        cap *= 2;

                nd = kmalloc(cap);
                if (!nd)
                        return -1;

                if (min->data)
                {
                        memcpy(nd, min->data, min->size);
                        kfree(min->data);
                }
                min->data = nd;
                min->capacity = cap;
        }

        memcpy(min->data + *off, buf, len);
        *off += len;
        if (*off > min->size)
                min->size = (uint32_t)*off;

        return len;
}

static int memfs_open(struct inode *inode, struct file *filp)
{
        struct memfs_inode *min;

        min = (struct memfs_inode *)inode->i_private;
        if (!min)
                return -1;

        if (filp->f_flags & O_TRUNC)
        {
                min->size = 0;
                filp->f_pos = 0;
        }

        return 0;
}

static int memfs_release(struct inode *inode, struct file *filp)
{
        (void)inode;
        (void)filp;

        return 0;
}

static const struct file_operations memfs_fops = {
    .read = memfs_read,
    .write = memfs_write,
    .open = memfs_open,
    .release = memfs_release,
};

static const struct inode_operations memfs_iops = {
    .lookup = vfs_generic_lookup,
    .create = memfs_create,
};

struct super_block *memfs_mount(void)
{
        struct super_block *sb;
        struct inode *root_inode;
        struct dentry *root_dentry;

        if (memfs_sb)
                return memfs_sb;

        sb = kmalloc(sizeof(struct super_block));
        if (!sb)
                return NULL;

        memset(sb, 0, sizeof(struct super_block));

        root_inode = memfs_alloc_inode(S_IFDIR | 0755);
        if (!root_inode)
        {
                kfree(sb);
                return NULL;
        }
        root_inode->i_op = &memfs_iops;

        root_dentry = vfs_new_dentry("/", root_inode, NULL);
        if (!root_dentry)
        {
                kfree(root_inode->i_private);
                kfree(root_inode);
                kfree(sb);
                return NULL;
        }
        sb->s_root = root_dentry;

        memfs_sb = sb;
        serial_write("[memfs] mounted at /home (writable)\n");

        return sb;
}
