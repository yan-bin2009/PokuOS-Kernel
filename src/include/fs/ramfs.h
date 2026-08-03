#ifndef _FS_RAMFS_H
#define _FS_RAMFS_H

#include <fs/vfs.h>

struct super_block *ramfs_mount(void *img);

#endif
