#ifndef _FS_MOUNT_H
#define _FS_MOUNT_H

#include <fs/vfs.h>

struct mount_node
{
        char *path;
        struct super_block *sb;
        struct mount_node *children[26];
        struct mount_node *next;
};

int vfs_mount(const char *path, struct super_block *sb);
struct super_block *vfs_find_mount(const char *path, const char **rest);

#endif
