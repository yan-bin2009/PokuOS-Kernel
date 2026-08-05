#include <fs/bdev.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/serial.h>

#define BDEV_MAX 8

static struct block_device *bdev_table[BDEV_MAX];
static int bdev_count = 0;

struct deferred_write
{
        uint64_t lba;
        uint8_t data[512];
        tier_t tier;
        struct deferred_write *next;
};

static struct deferred_write *write_pool = NULL;

int bdev_register(struct block_device *dev)
{
        if (!dev || bdev_count >= BDEV_MAX)
                return -1;

        bdev_table[bdev_count] = dev;
        bdev_count++;

        serial_write("[bdev] registered: ");
        serial_write(dev->name ? dev->name : "unknown");
        serial_write("\n");

        return 0;
}

int bdev_read(struct block_device *dev, uint64_t lba, void *buf,
              uint32_t nsec, tier_t tier)
{
        if (!dev || !dev->read)
                return -1;

        return dev->read(dev, lba, buf, nsec, tier);
}

int bdev_write(struct block_device *dev, uint64_t lba, const void *buf,
               uint32_t nsec, tier_t tier)
{
        if (!dev || !dev->write)
                return -1;

        return dev->write(dev, lba, buf, nsec, tier);
}

int bdev_write_deferred(struct block_device *dev, uint64_t lba,
                        const void *buf, tier_t tier)
{
        struct deferred_write *entry;

        (void)dev;

        entry = kmalloc(sizeof(struct deferred_write));
        if (!entry)
                return -1;

        entry->lba = lba;
        entry->tier = tier;
        memcpy(entry->data, buf, 512);
        entry->next = write_pool;
        write_pool = entry;

        return 0;
}

int bdev_flush(struct block_device *dev)
{
        struct deferred_write *entry;
        int ret;

        if (!dev || !dev->write)
                return -1;

        ret = 0;
        while (write_pool)
        {
                entry = write_pool;
                write_pool = entry->next;
                if (dev->write(dev, entry->lba, entry->data, 1,
                               entry->tier) != 0)
                {
                        ret = -1;
                }
                kfree(entry);
        }

        return ret;
}

struct block_device *bdev_lookup(const char *name)
{
        int i;

        for (i = 0; i < bdev_count; i++)
        {
                if (bdev_table[i] && bdev_table[i]->name &&
                    strcmp(bdev_table[i]->name, name) == 0)
                {
                        return bdev_table[i];
                }
        }

        return NULL;
}
