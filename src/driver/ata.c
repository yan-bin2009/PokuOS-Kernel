#include <driver/ata.h>
#include <fs/bdev.h>
#include <kernel/kstring.h>
#include <kernel/paging.h>
#include <kernel/ports.h>
#include <kernel/serial.h>
#include <stddef.h>

static struct ata_device dev_master;
static struct ata_device dev_slave;
static struct block_device bdev_master;
static struct block_device bdev_slave;

static void ata_wait_bsy(void)
{
        uint8_t status;

        do
        {
                status = inb(ATA_STATUS);
        } while (status & ATA_SR_BSY);
}

static void ata_wait_drq(void)
{
        uint8_t status;

        do
        {
                status = inb(ATA_STATUS);
        } while (!(status & ATA_SR_DRQ) && !(status & ATA_SR_ERR));
}

static void ata_select_drive(uint8_t master)
{
        uint8_t drive;

        drive = (master ? 0xE0 : 0xF0);
        outb(ATA_DRIVE, drive);
        ata_wait_bsy();
}

static void ata_read_buffer(void *buf, uint32_t count)
{
        uint32_t i;
        uint16_t *ptr;

        ptr = (uint16_t *)buf;
        for (i = 0; i < count / 2; i++)
        {
                ptr[i] = inw(ATA_DATA);
        }
}

static void ata_write_buffer(const void *buf, uint32_t count)
{
        uint32_t i;
        const uint16_t *ptr;

        ptr = (const uint16_t *)buf;
        for (i = 0; i < count / 2; i++)
        {
                outw(ATA_DATA, ptr[i]);
        }
}

int ata_identify(struct ata_device *dev, uint8_t master)
{
        uint8_t status;
        uint16_t buf[256];
        uint32_t i;

        if (!dev)
                return -1;

        ata_wait_bsy();
        ata_select_drive(master);

        outb(ATA_SECTOR_COUNT, 0);
        outb(ATA_LBA_LOW, 0);
        outb(ATA_LBA_MID, 0);
        outb(ATA_LBA_HIGH, 0);
        outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

        status = inb(ATA_STATUS);
        if (status == 0)
                return -1;

        ata_wait_bsy();

        status = inb(ATA_STATUS);
        if (status & ATA_SR_ERR)
                return -1;

        ata_wait_drq();
        ata_read_buffer(buf, 512);

        dev->present = 1;
        dev->master = master;
        dev->signature = buf[1];
        dev->capabilities = buf[49];
        dev->sectors = *((uint32_t *)(&buf[60]));

        memset(dev->model, 0, 41);
        for (i = 0; i < 20; i++)
        {
                uint16_t w = buf[27 + i];
                dev->model[i * 2] = w >> 8;
                dev->model[i * 2 + 1] = w & 0xFF;
        }
        for (i = 0; i < 40; i++)
        {
                if (dev->model[i] == ' ')
                {
                        dev->model[i] = '\0';
                        break;
                }
        }

        memset(dev->serial, 0, 21);
        for (i = 0; i < 10; i++)
        {
                uint16_t w = buf[10 + i];
                dev->serial[i * 2] = w >> 8;
                dev->serial[i * 2 + 1] = w & 0xFF;
        }
        for (i = 0; i < 20; i++)
        {
                if (dev->serial[i] == ' ')
                {
                        dev->serial[i] = '\0';
                        break;
                }
        }

        serial_write("[ATA] IDENTIFY: ");
        serial_write(dev->model);
        serial_write(" (");
        serial_write(dev->serial);
        serial_write(") ");
        serial_write_hex(dev->sectors);
        serial_write(" sectors\n");

        return 0;
}

int ata_read_sectors(struct ata_device *dev, uint32_t lba,
                     void *buf, uint32_t count)
{
        uint32_t i;
        uint8_t *ptr;

        if (!dev || !dev->present || !buf || count == 0)
                return -1;

        ptr = (uint8_t *)buf;

        for (i = 0; i < count; i++)
        {
                ata_wait_bsy();
                ata_select_drive(dev->master);

                outb(ATA_SECTOR_COUNT, 1);
                outb(ATA_LBA_LOW, lba & 0xFF);
                outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
                outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
                outb(ATA_DRIVE, (dev->master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
                outb(ATA_COMMAND, ATA_CMD_READ);

                ata_wait_drq();
                ata_read_buffer(ptr + i * 512, 512);

                lba++;
        }

        return 0;
}

int ata_write_sectors(struct ata_device *dev, uint32_t lba,
                      const void *buf, uint32_t count)
{
        uint32_t i;
        const uint8_t *ptr;

        if (!dev || !dev->present || !buf || count == 0)
                return -1;

        ptr = (const uint8_t *)buf;

        for (i = 0; i < count; i++)
        {
                ata_wait_bsy();
                ata_select_drive(dev->master);

                outb(ATA_SECTOR_COUNT, 1);
                outb(ATA_LBA_LOW, lba & 0xFF);
                outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
                outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
                outb(ATA_DRIVE, (dev->master ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F));
                outb(ATA_COMMAND, ATA_CMD_WRITE);

                ata_wait_drq();
                ata_write_buffer(ptr + i * 512, 512);

                lba++;
        }

        return 0;
}

static int ata_bdev_read(struct block_device *bdev, uint64_t lba,
                         void *buf, uint32_t nsec, tier_t tier)
{
        struct ata_device *dev;

        (void)tier;

        if (!bdev || !bdev->private)
                return -1;

        dev = (struct ata_device *)bdev->private;
        return ata_read_sectors(dev, (uint32_t)lba, buf, nsec);
}

static int ata_bdev_write(struct block_device *bdev, uint64_t lba,
                          const void *buf, uint32_t nsec, tier_t tier)
{
        struct ata_device *dev;

        (void)tier;

        if (!bdev || !bdev->private)
                return -1;

        dev = (struct ata_device *)bdev->private;
        return ata_write_sectors(dev, (uint32_t)lba, buf, nsec);
}

int ata_init(void)
{
        int ret;

        memset(&dev_master, 0, sizeof(dev_master));
        memset(&dev_slave, 0, sizeof(dev_slave));

        serial_write("[ATA] Initializing...\n");

        ret = ata_identify(&dev_master, 1);
        if (ret == 0 && dev_master.present)
        {
                bdev_master.name = "ata0";
                bdev_master.blocks = dev_master.sectors;
                bdev_master.block_size = 512;
                bdev_master.read = ata_bdev_read;
                bdev_master.write = ata_bdev_write;
                bdev_master.private = &dev_master;

                bdev_register(&bdev_master);
                serial_write("[ATA] Master registered as bdev\n");
        }

        ret = ata_identify(&dev_slave, 0);
        if (ret == 0 && dev_slave.present)
        {
                bdev_slave.name = "ata1";
                bdev_slave.blocks = dev_slave.sectors;
                bdev_slave.block_size = 512;
                bdev_slave.read = ata_bdev_read;
                bdev_slave.write = ata_bdev_write;
                bdev_slave.private = &dev_slave;

                bdev_register(&bdev_slave);
                serial_write("[ATA] Slave registered as bdev\n");
        }

        if (!dev_master.present && !dev_slave.present)
        {
                serial_write("[ATA] No device found\n");
                return -1;
        }

        return 0;
}
