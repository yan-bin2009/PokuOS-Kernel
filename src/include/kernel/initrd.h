#ifndef _KERNEL_INITRD_H
#define _KERNEL_INITRD_H

#include <stdint.h>
#include <kernel/vfs.h>   /* 改为 vfs.h，因为 fs.h 已经删了 */

typedef struct {
    uint32_t nfiles;
} initrd_header_t;

typedef struct {
    uint8_t magic;
    char name[64];
    uint32_t offset;
    uint32_t length;
} initrd_file_header_t;

struct dentry *initialise_initrd(uint32_t location);

#endif
