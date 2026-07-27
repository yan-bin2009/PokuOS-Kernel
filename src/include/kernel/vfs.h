#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <stdint.h>
#include <stddef.h>

/* 基础类型 */
typedef long ssize_t;
typedef long long loff_t;
typedef unsigned int umode_t;
#define __user

/* 文件类型 */
#define S_IFREG  0x8000
#define S_IFDIR  0x4000
#define S_IFCHR  0x2000
#define S_IFBLK  0x6000
#define S_IFIFO  0x1000
#define S_IFLNK  0xA000
#define S_IFMT   0xF000

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0100
#define O_TRUNC  0x0200

struct super_block;
struct inode;
struct dentry;
struct file;

struct file_operations {
    ssize_t (*read)(struct file *, char *, size_t, loff_t *);
    ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
    loff_t (*llseek)(struct file *, loff_t, int);
    int (*open)(struct inode *, struct file *);
    int (*release)(struct inode *, struct file *);
    int (*readdir)(struct file *, void *, int (*)(const char *, uint32_t, loff_t));
};

struct inode_operations {
    struct dentry *(*lookup)(struct inode *, struct dentry *);
    int (*create)(struct inode *, struct dentry *, umode_t);
    int (*mkdir)(struct inode *, struct dentry *, umode_t);
    int (*unlink)(struct inode *, struct dentry *);
    int (*rmdir)(struct inode *, struct dentry *);
    int (*link)(struct dentry *, struct inode *, struct dentry *);
    int (*symlink)(struct inode *, struct dentry *, const char *);
};

struct super_operations {
    void (*put_super)(struct super_block *);
    int (*sync_fs)(struct super_block *);
};

struct super_block {
    uint32_t s_dev;
    uint32_t s_blocksize;
    struct inode *s_root;
    const struct super_operations *s_op;
    void *s_fs_info;
};

struct inode {
    uint32_t i_ino;
    uint32_t i_mode;
    uint32_t i_size;
    uint32_t i_uid;
    uint32_t i_gid;
    uint32_t i_links;
    struct super_block *i_sb;
    const struct inode_operations *i_op;
    const struct file_operations *i_fop;
    void *i_private;
    struct dentry *i_children;
};

struct dentry {
    char *d_name;
    struct inode *d_inode;
    struct dentry *d_parent;
    struct dentry *d_next;
    struct dentry *d_hash_next;
};

struct file {
    struct dentry *f_dentry;
    loff_t f_pos;
    uint32_t f_flags;
    const struct file_operations *f_op;
    void *f_private;
};

extern struct dentry *vfs_root;

int vfs_mount_root(struct inode *root_inode);
struct dentry *vfs_lookup(const char *path);
struct file *vfs_open(const char *path, uint32_t flags);
ssize_t vfs_read(struct file *filp, char *buf, size_t len);
ssize_t vfs_write(struct file *filp, const char *buf, size_t len);
int vfs_close(struct file *filp);
int vfs_readdir(struct file *filp, int (*filldir)(const char *, uint32_t, loff_t));

struct inode *vfs_new_inode(uint32_t mode);
struct dentry *vfs_new_dentry(const char *name, struct inode *inode, struct dentry *parent);
void vfs_free_dentry(struct dentry *dentry);

#endif
