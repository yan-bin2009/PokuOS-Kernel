#include <kernel/vfs.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <driver/vga.h>
#include <stddef.h>
#include <kernel/kstring.h>
struct dentry *vfs_root;

/* 新建 inode */
struct inode *vfs_new_inode(uint32_t mode)
{
	struct inode *inode;

	inode = (struct inode *)kmalloc(sizeof(struct inode));
	if (!inode)
		return NULL;
	memset(inode, 0, sizeof(struct inode));
	inode->i_mode = mode;
	inode->i_links = 1;
	return inode;
}

/* 新建 dentry，并插入父目录的子链表 */
struct dentry *vfs_new_dentry(const char *name, struct inode *inode,
			      struct dentry *parent)
{
	struct dentry *dentry;
	size_t len;

	dentry = (struct dentry *)kmalloc(sizeof(struct dentry));
	if (!dentry)
		return NULL;
	memset(dentry, 0, sizeof(struct dentry));
	len = strlen(name) + 1;
	dentry->d_name = (char *)kmalloc(len);
	if (!dentry->d_name) {
		kfree(dentry);
		return NULL;
	}
	strcpy(dentry->d_name, name);
	dentry->d_inode = inode;
	dentry->d_parent = parent;
	if (parent) {
		dentry->d_next = parent->d_inode->i_children;
		parent->d_inode->i_children = dentry;
	}
	return dentry;
}

void vfs_free_dentry(struct dentry *dentry)
{
	if (!dentry)
		return;
	if (dentry->d_name)
		kfree(dentry->d_name);
	kfree(dentry);
}

/* 挂载根文件系统 */
int vfs_mount_root(struct inode *root_inode)
{
	struct dentry *root_dentry;

	if (!root_inode || !(root_inode->i_mode & S_IFDIR))
		return -1;
	root_dentry = vfs_new_dentry("/", root_inode, NULL);
	if (!root_dentry)
		return -1;
	vfs_root = root_dentry;
	vga_write("[VFS] Root mounted.\n");
	return 0;
}

/* 路径查找（仅支持绝对路径，无缓存） */
struct dentry *vfs_lookup(const char *path)
{
	char *path_copy, *token, *next;
	struct dentry *cur;
	size_t len;

		if (!path || !vfs_root)
		return NULL;
	if (*path == '/')
		path++;
	if (*path == '\0')
		return vfs_root;

	len = strlen(path) + 1;
	path_copy = (char *)kmalloc(len);
	if (!path_copy)
		return NULL;
	strcpy(path_copy, path);

	cur = vfs_root;
	token = path_copy;

	while ((next = strchr(token, '/')) != NULL) {
		*next = '\0';
		if (cur->d_inode->i_op && cur->d_inode->i_op->lookup) {
			/* 创建临时 dentry，只存名字，不插入链表 */
			struct dentry *tmp = (struct dentry *)kmalloc(sizeof(struct dentry));
			if (!tmp) {
				kfree(path_copy);
				return NULL;
			}
			memset(tmp, 0, sizeof(struct dentry));
			tmp->d_name = (char *)kmalloc(strlen(token) + 1);
			if (!tmp->d_name) {
				kfree(tmp);
				kfree(path_copy);
				return NULL;
			}
			strcpy(tmp->d_name, token);
			/* lookup 会返回目标 dentry（可能新分配），我们直接用它 */
			cur = cur->d_inode->i_op->lookup(cur->d_inode, tmp);
			/* 释放临时 dentry（lookup 实现不会保留它） */
			kfree(tmp->d_name);
			kfree(tmp);
			if (!cur) {
				kfree(path_copy);
				return NULL;
			}
		} else {
			kfree(path_copy);
			return NULL;
		}
		token = next + 1;
	}

	/* 处理最后一个组件 */
	if (strlen(token) > 0) {
		if (cur->d_inode->i_op && cur->d_inode->i_op->lookup) {
			struct dentry *tmp = (struct dentry *)kmalloc(sizeof(struct dentry));
			if (!tmp) {
				kfree(path_copy);
				return NULL;
			}
			memset(tmp, 0, sizeof(struct dentry));
			tmp->d_name = (char *)kmalloc(strlen(token) + 1);
			if (!tmp->d_name) {
				kfree(tmp);
				kfree(path_copy);
				return NULL;
			}
			strcpy(tmp->d_name, token);
			cur = cur->d_inode->i_op->lookup(cur->d_inode, tmp);
			kfree(tmp->d_name);
			kfree(tmp);
			if (!cur) {
				kfree(path_copy);
				return NULL;
			}
		} else {
			kfree(path_copy);
			return NULL;
		}
	}

	kfree(path_copy);
	return cur;
}
/* 打开文件 */
struct file *vfs_open(const char *path, uint32_t flags)
{
	struct dentry *dentry;
	struct inode *inode;
	struct file *filp;

	dentry = vfs_lookup(path);
	if (!dentry)
		return NULL;
	inode = dentry->d_inode;
	if (!inode)
		return NULL;

	filp = (struct file *)kmalloc(sizeof(struct file));
	if (!filp)
		return NULL;
	memset(filp, 0, sizeof(struct file));
	filp->f_dentry = dentry;
	filp->f_pos = 0;
	filp->f_flags = flags;
	filp->f_op = inode->i_fop;

	if (filp->f_op && filp->f_op->open) {
		if (filp->f_op->open(inode, filp) != 0) {
			kfree(filp);
			return NULL;
		}
	}
	return filp;
}

ssize_t vfs_read(struct file *filp, char *buf, size_t len)
{
	if (!filp || !filp->f_op || !filp->f_op->read)
		return -1;
	return filp->f_op->read(filp, buf, len, &filp->f_pos);
}

ssize_t vfs_write(struct file *filp, const char *buf, size_t len)
{
	if (!filp || !filp->f_op || !filp->f_op->write)
		return -1;
	return filp->f_op->write(filp, buf, len, &filp->f_pos);
}

int vfs_close(struct file *filp)
{
	int ret = 0;

	if (!filp)
		return -1;
	if (filp->f_op && filp->f_op->release)
		ret = filp->f_op->release(filp->f_dentry->d_inode, filp);
	kfree(filp);
	return ret;
}

int vfs_readdir(struct file *filp, int (*filldir)(const char *, uint32_t, loff_t))
{
	if (!filp || !filp->f_op || !filp->f_op->readdir)
		return -1;
	return filp->f_op->readdir(filp, NULL, filldir);
}
