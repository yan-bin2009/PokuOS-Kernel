#ifndef _FS_TARFS_H
#define _FS_TARFS_H

#include <fs/bdev.h>
#include <fs/vfs.h>

struct super_block *tarfs_mount(struct block_device *dev);

#endif
