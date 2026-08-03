#ifndef _DRIVER_ATA_H
#define _DRIVER_ATA_H

#include <stdint.h>

/* ATA 主通道端口 */
#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW    0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HIGH   0x1F5
#define ATA_DRIVE      0x1F6
#define ATA_COMMAND    0x1F7
#define ATA_STATUS     0x1F7
#define ATA_CONTROL    0x3F6

/* ATA 命令 */
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ     0x20
#define ATA_CMD_WRITE    0x30

/* ATA 状态位 */
#define ATA_SR_BSY  0x80
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

struct ata_device
{
        uint8_t present;
        uint16_t signature;
        uint16_t capabilities;
        uint32_t sectors;
        char model[41];
        char serial[21];
        uint8_t master;
};

int ata_init(void);
int ata_identify(struct ata_device *dev, uint8_t master);
int ata_read_sectors(struct ata_device *dev, uint32_t lba,
                     void *buf, uint32_t count);
int ata_write_sectors(struct ata_device *dev, uint32_t lba,
                      const void *buf, uint32_t count);

#endif
