#ifndef _FS_BDEV_H
#define _FS_BDEV_H

#include <fs/fs_types.h>
#include <stdint.h>

struct block_device
{
        const char *name;
        uint32_t blocks;
        uint32_t block_size;
        int (*read)(struct block_device *, uint64_t lba, void *buf,
                    uint32_t nsec, tier_t tier);
        int (*write)(struct block_device *, uint64_t lba, const void *buf,
                     uint32_t nsec, tier_t tier);
        void *private;
};

int bdev_register(struct block_device *dev);
int bdev_read(struct block_device *dev, uint64_t lba, void *buf,
              uint32_t nsec, tier_t tier);
int bdev_write(struct block_device *dev, uint64_t lba, const void *buf,
               uint32_t nsec, tier_t tier);
int bdev_write_deferred(struct block_device *dev, uint64_t lba,
                        const void *buf, tier_t tier);
int bdev_flush(struct block_device *dev);
struct block_device *bdev_lookup(const char *name);

#endif
